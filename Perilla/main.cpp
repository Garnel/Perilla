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
6  * 7.777 - 8.8
extern sin(x)

def bar(a)
    a + 100

def foo(x y)
    sin(x) * bar(y)

1 + foo(2, 3)+(4 + 5.5555)* 6  * 7.777 - 8.8
sin(0)
    
def test(x) (1+2+x) * (x + (1+2))
)CODE";

//    string src = "def test(x) (123+2+x) * (x + (123+2))";

    shared_ptr<Lexer> lexer = make_shared<StringLexer>(src);
//    Token token = lexer->NextToken();
//    while (token != Token::Eof) {
//        cout << token << endl;
//        token = lexer->NextToken();
//    }
    
    ASTGenerator astgen(lexer);
    astgen.Run();
    astgen.PrintAST();
    astgen.CodeGen();
}
