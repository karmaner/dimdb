#!/bin/bash
flex --outfile token.lex.cpp --header-file=token.lex.h token.lex
`which bison` -d --output syntax_tree.yacc.cpp syntax_tree.yacc --defines=syntax_tree.yacc.h
