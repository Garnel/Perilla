#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Token.h"
#include "OperatorPrecedence.h"
#include <exception>
#include "Utils.h"
#include "Lexer.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/IRBuilder.h"

using namespace std;
using namespace llvm;
using namespace Perilla;

namespace Perilla {
//
//class CompileException: exception
//{
//public:
//    CompileException(string message)
//    :errorMsg(message) {}
//    
//    const char* what() const noexcept
//    {
//        return errorMsg.c_str();
//    }
//
//private:
//    string errorMsg;
//};
    
static LLVMContext context;
static IRBuilder<> builder {context};
static std::unique_ptr<Module> module = llvm::make_unique<Module>("Perilla jit", context);
static std::map<std::string, Value *> symbolTable;

Value *LogErrorV(const string &msg) {
    cout << msg << endl;
    return nullptr;
}
    
struct ASTNode {
    virtual string GetString() = 0;
    virtual ~ASTNode() = default;
    
    virtual Value *CodeGen() = 0;
};

struct ExprAST: ASTNode {
    virtual string GetString() override
    {
        return "Expr";
    }
    
};

struct NumberExprAST: ExprAST
{
    double value;
    
    NumberExprAST(double val): value(val) {}
    
    virtual string GetString() override
    {
        return "Number Expr: " + to_string(value);
    }
    
    virtual Value *CodeGen() override
    {
        return ConstantFP::get(context, APFloat(value));
    }
};

struct VariableExprAST: ExprAST
{
    string variable;
    
    VariableExprAST(string var): variable(move(var)) {}
    
    virtual string GetString() override
    {
        return "Variable Expr: " + variable;
    }
    
    virtual Value *CodeGen() override
    {
        Value *v = symbolTable[variable];
        if (!v) {
            return LogErrorV("Unknow variable name");
        }
        return v;
    }
};
    
struct BinaryExprAST: ExprAST
{
    char op;
    shared_ptr<ExprAST> left, right;

    BinaryExprAST(char ch, shared_ptr<ExprAST> lhs, shared_ptr<ExprAST> rhs)
    :op(ch), left(lhs), right(rhs) {}
    
    virtual string GetString() override
    {
        return "Binary Expr: " + string(1, op);
    }
    
    virtual Value *CodeGen() override
    {
        Value *lhs = left->CodeGen();
        Value *rhs = right->CodeGen();
        
        if (!lhs || !rhs) {
            // TODO handle error
            return nullptr;
        }
        
        switch (op)
        {
            case '+':
                return builder.CreateFAdd(lhs, rhs, "addtmp");
            case '-':
                return builder.CreateFSub(lhs, rhs, "subtmp");
            case '*':
                return builder.CreateFMul(lhs, rhs, "multmp");
            case '<':
                lhs = builder.CreateFCmpULT(lhs, rhs, "cmptmp");
                // convert bool 0/1 to double 0.0/1.0
                return builder.CreateUIToFP(lhs, Type::getDoubleTy(context), "booltmp");
            default:
                return LogErrorV("Invalid binary operator");
        }
    }
};

struct CallExprAST: ExprAST
{
    string callee;
    vector<shared_ptr<ExprAST>> args;
    
    CallExprAST(string name, vector<shared_ptr<ExprAST>> arglist)
    : callee(move(name)), args(move(arglist)) {}
    
    virtual string GetString() override
    {
        return "Call Function: " + callee;
    }
    
    virtual Value *CodeGen() override
    {
        Function *func = module->getFunction(callee);
        if (!func) {
            return LogErrorV("Unknow function referenced");
        }
        
        // argument match error
        if (func->arg_size() != args.size()) {
            return LogErrorV("Incorrect arguments size");
        }
        
        vector<Value *> argList;
        for (size_t idx = 0; idx < args.size(); ++idx) {
            Value *val = args[idx]->CodeGen();
            if (!val) {
                return LogErrorV("Evaluating argument " + to_string(idx) + " of function " + callee + " fails");
            }
            argList.push_back(val);
        }
        
        return builder.CreateCall(func, argList, "calltmp");
    }
};
    
struct PrototypeAST: ASTNode
{
    string name;
    vector<string> args;
    
    PrototypeAST(string funcName, vector<string> argList)
    :name(move(funcName)), args(move(argList)) {}
    
    
    virtual string GetString() override
    {
        string buffer = "Prototype: " + name + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i != 0) buffer += " ";
            buffer += args[i];
        }
        buffer += ")";
        return buffer;
    }
    
    Function *CodeGen() override
    {
        // Make the function type double(double, double)
        vector<Type*> doubles(args.size(), Type::getDoubleTy(context));
        
        FunctionType *ft = FunctionType::get(Type::getDoubleTy(context), doubles, false);
        Function *f = Function::Create(ft, Function::ExternalLinkage, name, module.get());
        
        size_t idx = 0;
        for (auto &Arg : f->args()) {
            Arg.setName(args[idx++]);
        }
        
        return f;
    }
};
    
struct FunctionAST: ASTNode
{
    shared_ptr<PrototypeAST> prototype;
    shared_ptr<ExprAST> body;
    
    FunctionAST(shared_ptr<PrototypeAST> proto, shared_ptr<ExprAST> b)
    :prototype(proto), body(b) {}

    virtual string GetString() override
    {
        return "Function Definition: " + (prototype ? prototype->GetString() : "Anonymouse");
    }
    
    Function *CodeGen() override
    {
        // First, check for an existing function from a previous 'extern' declaration.
        Function *func = module->getFunction(prototype->name);
        
        if (!func) {
            func = prototype->CodeGen();
        }
        
        if (!func) {
            return nullptr;
        }
        
        if (!func->empty()) {
            return (Function*)LogErrorV("Function cannot be redefined");
        }
        
        BasicBlock *bb = BasicBlock::Create(context, "entry", func);
        builder.SetInsertPoint(bb);
        
        symbolTable.clear();
        for (auto &arg: func->args()) {
            symbolTable[arg.getName()] = &arg;
        }
        
        if (Value *retVal = body->CodeGen()) {
            // conplete function
            builder.CreateRet(retVal);
            
            // Validate the generated code, checking for consistency.
            verifyFunction(*func);
            
            return func;
        }
        
        func->eraseFromParent();
        return (Function*)LogErrorV("Error reading body, remove function");
    }
};

class ASTGenerator
{
public:
    ASTGenerator(shared_ptr<Lexer> _lexer): lexer(_lexer), current(Token::EofToken) {}

    void Run() {
        GetCurrent();
        while (current != Token::EofToken) {
            if (current == Token{';'}) {
                // ignore toplevel ;
                GetCurrent(); // consume ;
                continue;
            }

            switch (current.GetType()) {
                case Token::Type::Def:
                    astNodes.push_back(ParseDefinition());
                    break;
                case Token::Type::Extern:
                    astNodes.push_back(ParseExtern());
                    break;
                default:
                    astNodes.push_back(ParseToplevel());
                    break;
            }
        }
    }
    
    void PrintAST() const
    {
        for (auto& node: astNodes) {
            cout << node->GetString() << endl;
        }
    }
    
    void CodeGen() const
    {
        for (auto &node: astNodes) {
            node->CodeGen();
//            if (auto *ir = node->CodeGen()) {
//                ir->dump();
//            }
        }
        module->dump();
    }

    shared_ptr<ExprAST> ParsePrimary()
    {
        assert(current != Token::EofToken);

        if (current.IsNumber()) {
            double value = current.GetNumeric();
            GetCurrent();
            return make_shared<NumberExprAST>(value);
        } else if (current == Token{'('}) {
            // '(' epxression ')'
            GetCurrent(); // consume '('
            auto expr = ParseExpr();
            
            if (current != Token{')'}) {
                HandleError("Expecting ')'");  // continue parsing
            } else {
                GetCurrent();
            }
            return expr;
        } else if (current.IsIdent()) {
            // look forward to determine it's a variable or a function call
            auto previous = current;
            if (GetCurrent() && current == Token{'('}) {
                // function call
                GetCurrent();
                auto args = ParseArguments();
                
                if (current != Token{')'}) {
                    HandleError("Expecting ')'");
                } else {
                    GetCurrent();
                }
                return make_shared<CallExprAST>(previous.GetContent(), move(args));
            }
            return make_shared<VariableExprAST>(previous.GetContent());
        }
        
        HandleError("Expecting an expression");
        return nullptr;
    }
    
    vector<shared_ptr<ExprAST>> ParseArguments()
    {
        vector<shared_ptr<ExprAST>> args;
        // empty arguments
        if (current == Token{')'}) {
            return args;
        }

        while (true) {
            args.push_back(ParseExpr());

            if (current == Token{')'}) {
                return args;
            }

            if (current == Token{','}) {
                GetCurrent(); // consume ','
            } else {
                HandleError("Expecting ','");
                return args;
            }
        }
    }
    
    shared_ptr<ExprAST> ParseExpr()
    {
        auto lhs = ParsePrimary();
        return ParseBinRhs(0, lhs);
    }
    
    shared_ptr<ExprAST> ParseBinRhs(int precedence, shared_ptr<ExprAST> lhs)
    {
        auto previous = current;
        if (!previous.IsUnknown()) {
//            HandleError("Expecting a binary operator");
            return lhs;
        }

        if (BinaryOperatorPrecedence::Support(previous.GetChar())) {
            int prevPrec = BinaryOperatorPrecedence::Get(previous.GetChar());
            if (prevPrec < precedence) {
                return lhs;
            } else {
                GetCurrent(); // consume binary operator
                auto rhs = ParsePrimary();
                if (current.IsUnknown() && BinaryOperatorPrecedence::Support(current.GetChar())) {
                    int curPrec = BinaryOperatorPrecedence::Get(current.GetChar());
                    if (prevPrec < curPrec) {
                        rhs = ParseBinRhs(prevPrec + 1, rhs);
                    }
                }
                lhs = make_shared<BinaryExprAST>(previous.GetChar(), lhs, rhs);
                return ParseBinRhs(precedence, lhs);
            }
        } else {
//            HandleError("Unsupported binary operator " + previous.GetContent());
            return lhs;
        }
    }
    
    shared_ptr<PrototypeAST> ParsePrototype()
    {
        // id '(' id* ')'
        assert(current.IsIdent());

        auto funcToken = current;
        GetCurrent(); // consume the function name
        if (current != Token{'('}) {
            HandleError("Expection '('");
            return nullptr;
        }
        GetCurrent(); // consume (
        
        vector<string> args;
        if (current != Token{')'}) {
            while (true) {
                if (!current.IsIdent()) {
                    HandleError("Expecting an ident");
                    break;
                }
                args.push_back(current.GetContent());
                GetCurrent(); //consume argument name
                
                if (current == Token{')'}) {
                    GetCurrent(); // consume )
                    break;
                }
            }
        } else {
            GetCurrent(); // consume )
        }
        
        return make_shared<PrototypeAST>(funcToken.GetContent(), args);
    }
    
    shared_ptr<FunctionAST> ParseDefinition()
    {
        assert(current.IsDef());
        
        GetCurrent(); // consume def
        auto proto = ParsePrototype();
        auto body = ParseExpr();
        return make_shared<FunctionAST>(proto, body);
    }
    
    shared_ptr<PrototypeAST> ParseExtern()
    {
        assert(current.IsExtern());
        
        GetCurrent(); // consume extern
        return ParsePrototype();
    }
    
    shared_ptr<FunctionAST> ParseToplevel()
    {
        // make a anonymouse prototype
        // anonymouse nullary function
        auto proto = make_shared<PrototypeAST>("__anon_expr_" + GenerateRandom(10), vector<string>());
        return make_shared<FunctionAST>(proto, ParseExpr());
    }
    
    vector<shared_ptr<ASTNode>> GetASTNodes() const
    {
        return astNodes;
    }

private:
    void HandleError(string errorMessage)
    {
        cout << errorMessage << endl;
    }
    
    bool GetCurrent()
    {
        if (lexer) {
            current = lexer->NextToken();
            return current != Token::EofToken;
        }
        return false;
    }
    
    shared_ptr<Lexer> lexer;
    vector<shared_ptr<ASTNode>> astNodes;
    Token current;
};

};
