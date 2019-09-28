/* This file is part of the Practical programming langauge. https://github.com/Practical/practical-sa
 *
 * This file is file is copyright (C) 2018-2019 by its authors.
 * You can see the file's authors in the AUTHORS file in the project's home repository.
 *
 * This is available under the Boost license. The license's text is available under the LICENSE file in the project's
 * home directory.
 */
#include <iostream>
#include <variant>

#include <unistd.h>

#include "parser.h"
#include "mmap.h"

size_t indentWidth = 3;

std::ostream &indent( std::ostream &out, size_t depth ) {
    for( unsigned i=0; i<depth*indentWidth; ++i )
        out<<" ";

    return out;
}

using namespace NonTerminals;

void dumpIdentifier( const NonTerminals::Identifier &id, size_t depth ) {
    indent(std::cout, depth) << "Identifier "<<*id.identifier<<"\n";
}

void dumpType( const NonTerminals::Type &type, size_t depth ) {
    dumpIdentifier(type.type, depth);
}

void dumpParseTree( const NonTerminals::Expression &node, size_t depth=0 );

void dumpParseTree( const FunctionArguments &args, size_t depth=0 ) {
    for( const auto &i : args.arguments ) {
        dumpParseTree( i, depth+1 );
    }
}

void dumpParseTree( const NonTerminals::Expression &node, size_t depth ) {
    struct Visitor {
        size_t depth;
        std::ostream &out;

        Visitor( size_t depth, std::ostream &out ) : depth(depth), out(out) {}

        void operator()( const std::unique_ptr<::NonTerminals::CompoundExpression> &compound ) {}
        void operator()( const Literal &literal ) {
            indent(out, depth) << "Literal "<<literal.token<<"\n";
        }

        void operator()( const Identifier &id ) {
            dumpIdentifier( id, depth );
        }

        void operator()( const NonTerminals::Expression::UnaryOperator &op ) {
            indent( out, depth )<<"Unary "<<*op.op<<"\n";
            dumpParseTree( *op.operand, depth+1 );
        }

        void operator()( const NonTerminals::Expression::BinaryOperator &op ) {
            indent( out, depth )<<"Binary "<<*op.op<<"\n";
            indent( out, depth )<<"Operand 1:\n";
            dumpParseTree( *op.operand1, depth+1 );
            indent( out, depth )<<"Operand 2:\n";
            dumpParseTree( *op.operand2, depth+1 );
        }

        void operator()( const NonTerminals::Expression::FunctionCall &func ) {
            indent(out, depth) << "Function call:\n";
            dumpParseTree( *func.expression, depth+1 );
            indent(out, depth) << "Arguments:\n";
            dumpParseTree( func.arguments, depth+1 );
        }

        void operator()( const NonTerminals::Type &type ) {
            indent(out, depth) << "Type:\n";
            dumpType( type, depth+1 );
        }
    };

    std::visit( Visitor( depth, std::cout ), node.value );
}

void help() {
    std::cout <<
            "Practiparse: exercise the Practical parser\n"
            "Usage: practiparse filename\n"
            "Options:\n"
            "-c\tArgument is the actual program source, instead of the file name\n"
            "-W\tSource is the whole program, rather than a single expression\n"
            "-i<num>\tSet the per-level indent mount\n";
}

int main(int argc, char *argv[]) {
    bool singleExpression = true;
    bool argumentSource = false;
    int opt;

    while( (opt=getopt(argc, argv, "Wchi:?")) != -1 ) {
        switch( opt ) {
        case 'W':
            singleExpression = false;
            break;
        case 'c':
            argumentSource = true;
            break;
        case 'i':
            indentWidth = strtoul( optarg, nullptr, 10 );
            break;
        case '?':
            help();
            return 0;
        default:
            std::cerr << "Invalid option '-" << static_cast<char>(opt) << "'. Use -? for help." << std::endl;
            help();
            return 1;
        }
    }

    if( optind == argc ) {
        std::cerr << "No argument" << std::endl;
        help();
        return 1;
    }

    try {
        std::unique_ptr< Mmap<MapMode::ReadOnly> > fileSource;
        String textSource;

        if( argumentSource ) {
            textSource = String(argv[optind]);
        } else {
            fileSource = safenew< Mmap<MapMode::ReadOnly> >( (argv[optind]) );
            textSource = fileSource->getSlice<const char>();
        }

        // Tokenize
        auto tokens = Tokenizer::Tokenizer::tokenize( textSource );

        // Parse
        if( singleExpression ) {
            NonTerminals::Expression exp;
            exp.parse( tokens );
            std::cout<<"Successfully parsed. Dumping parse tree:\n";
            dumpParseTree( exp );
        } else {
            NonTerminals::Module module;
            module.parse( tokens );
            std::cout<<"Successfully parsed. Dumping parse tree:\n";
        }
    } catch(std::exception &error) {
        std::cerr << "Parsing failed: " << error.what() << "\n";

        return 1;
    }
}
