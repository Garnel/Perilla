#pragma once
#include <string>
#include <memory>
#include <vector>
#include "Token.h"
#include "OperatorPrecedence.h"
#include <exception>
#include "Utils.h"

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
        return "Binary Expr: " + string{1, op};
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
        return "Prototype: " + name;
    }
    
    Function *CodeGen()
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
        return "Function Definition: " + (prototype ? prototype->name : "Anonymouse");
    }
    
    Function *CodeGen()
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
    ASTGenerator(vector<shared_ptr<Token>> ts): tokens(move(ts)), cur(0)  {}
    
    void Run() {
        while (cur < tokens.size()) {
            if (*CurrentToken() == Token{';'}) {
                // ignore toplevel ;
                ++cur;
                continue;
            }

            switch (CurrentToken()->GetType()) {
                case Token::Def:
                    astNodes.push_back(ParseDefinition());
                    break;
                case Token::Extern:
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
            if (auto *ir = node->CodeGen()) {
                ir->dump();
            }
        }
        module->dump();
    }

    shared_ptr<ExprAST> ParsePrimary()
    {
        auto token = CurrentToken();
        if (!token) {
            // TODO handle error, unexpected end
            return nullptr;
        }
        auto type = token->GetType();
        if (type == Token::Type::Number) {
            // number expression
            ++cur;
            return make_shared<NumberExprAST>(token->GetNumeric());
        } else if (type == Token::Type::Unknown && token->GetChar() == '(') {
            // '(' epxression ')'
            ++cur; // consume '('
            auto expr = ParseExpr();
            if (*CurrentToken() != Token{')'}) {
                // TODO handle error
                return nullptr;
            } else {
                ++cur; // consume ')'
                return expr;
            }
        } else if (type == Token::Type::Ident) {
            auto next = NextToken();
            if (next != nullptr && *next == Token{'('}) {
                // function call
                ++cur; // consume function name
                ++cur; // consume '('
                auto args = ParseArguments();
                if (*CurrentToken() != Token{')'}) {
                    // TODO handle error, expecting )
                } else {
                    ++cur; // consume ')'
                    return make_shared<CallExprAST>(token->GetContent(), move(args));
                }
            } else {
                ++cur;
                return make_shared<VariableExprAST>(token->GetContent());
            }
        }
        
        // TODO handle error, unknown token when expecting an expression
        return nullptr;
    }
    
    vector<shared_ptr<ExprAST>> ParseArguments()
    {
        vector<shared_ptr<ExprAST>> args;
        // empty
        if (*CurrentToken() == Token{')'}) {
            return args;
        }

        while (true) {
            args.push_back(ParseExpr());

            auto token = CurrentToken();
            if (*token == Token{')'}) {
                return args;
            }

            if (*token == Token{','}) {
                ++cur; // consume ','
            } else {
                // TODO handler error, expecting ,
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
        auto prev = CurrentToken();
        if (!prev || !prev->IsUnknown()) {
            return lhs;
        }

        if (BinaryOperatorPrecedence::Support(prev->GetChar())) {
            int prevPrec = BinaryOperatorPrecedence::Get(prev->GetChar());
            if (prevPrec < precedence) {
                return lhs;
            } else {
                ++cur; // consume operator
                auto rhs = ParsePrimary();
                auto cur = CurrentToken();
                if (cur && cur->IsUnknown() && BinaryOperatorPrecedence::Support(cur->GetChar())) {
                    int curPrec = BinaryOperatorPrecedence::Get(cur->GetChar());
                    if (prevPrec < curPrec) {
                        rhs = ParseBinRhs(prevPrec + 1, rhs);
                    }
                }
                lhs = make_shared<BinaryExprAST>(prev->GetChar(), lhs, rhs);
                return ParseBinRhs(precedence, lhs);
            }
        } else {
            return lhs;
        }
    }
    
    shared_ptr<PrototypeAST> ParsePrototype()
    {
        // id '(' id* ')'
        auto funcToken = CurrentToken();
        if (!funcToken->IsIdent()) {
            // TODO expecting function name
            return nullptr;
        }
        
        ++cur; // consume func name
        if (*CurrentToken() != Token{'('}) {
            // TODO expecting (
            return nullptr;
        }
        ++cur; // consume (
        
        vector<string> argNames;
        if (*CurrentToken() != Token{')'}) {
            while (true) {
                if (!CurrentToken()->IsIdent()) {
                    // TODO expecting argument name
                    break;
                }
                argNames.push_back(CurrentToken()->GetContent());
                ++cur; // consume the argument name
                
                if (*CurrentToken() == Token{')'}) {
                    ++cur; // consume )
                    break;
                }
            }
        } else {
            ++cur; // consume )
        }
        
        return make_shared<PrototypeAST>(funcToken->GetContent(), argNames);
    }
    
    shared_ptr<FunctionAST> ParseDefinition()
    {
        if (!CurrentToken()->IsDef()) {
            // TODO expecting def
            return nullptr;
        }
        
        ++cur; // consume def
        auto proto = ParsePrototype();
        auto body = ParseExpr();
        return make_shared<FunctionAST>(proto, body);
    }
    
    shared_ptr<PrototypeAST> ParseExtern()
    {
        if (!CurrentToken()->IsExtern()) {
            // TODO expecting extern
            return nullptr;
        }
        
        ++cur; // consume extern
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
    inline shared_ptr<Token> CurrentToken() const {
        return cur < tokens.size() ? tokens[cur] : nullptr;
    }
    
    inline shared_ptr<Token> NextToken() const {
        return cur + 1 < tokens.size() ? tokens[cur + 1] : nullptr;
    }
    
    vector<shared_ptr<Token>> tokens;
    size_t cur;
    vector<shared_ptr<ASTNode>> astNodes;
};

};
