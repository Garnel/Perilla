#include "Lexer.h"
#include "AST.h"

using namespace Perilla;

int main()
{
//    string src = R"CODE(
//# Compute the x'th fibonacci number.
//def fib(x)
//  if x < 3 then
//    1
//  else
//    fib(x-1)+fib(x-2)
//
//# This expression will compute the 40th number.
//fib(5)
//)CODE";
    
    string src = R"CODE(
extern sin(x)

def bar(a)
    a + 100

def foo(x y)
    sin(x) * bar(y)

1 + foo(2, 3)+(4 + 5.5555)* 6  * 7.777 - 8.8
sin(0)
)CODE";
    
    StringLexer lexer(src);
    lexer.Run();
    lexer.PrintTokens();
    
    ASTGenerator astgen(lexer.getTokens());
    astgen.Run();
    astgen.PrintAST();
    astgen.CodeGen();
}
