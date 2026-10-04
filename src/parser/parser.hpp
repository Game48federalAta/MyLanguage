#pragma once

#include <variant>

#include "../Allocator/arena.hpp"
#include "../tokenizer/tokenizer.hpp"


struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;

struct NodeTermParen
{
    NodeExpr* expr;
};


struct NodeBinExprSub
{
    NodeExpr* lhs;//left hand size
    NodeExpr* rhs;//right hand size 
};

struct NodeBinExprDiv
{
    NodeExpr* lhs;//left hand size
    NodeExpr* rhs;//right hand size 
};

struct NodeBinExprAdd
{
    NodeExpr* lhs;//left hand size
    NodeExpr* rhs;//right hand size 
};

struct NodeBinExprMulti
{
    NodeExpr* lhs;//left hand size
    NodeExpr* rhs;//right hand size 
};


struct NodeBinExpr{
    std::variant<NodeBinExprAdd*,NodeBinExprMulti*,NodeBinExprDiv*,NodeBinExprSub*>var;
};




struct NodeTerm
{
    std::variant<NodeTermIntLit*,NodeTermIdent*,NodeTermParen*>var;
};


struct NodeExpr {
    std::variant<NodeTerm*,NodeBinExpr*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};
struct NodeStmt;

struct NodeScope{
    std::vector<NodeStmt*>stmts;

};

struct NodeStmtIf
{
    NodeExpr* expr;
    NodeScope* scope;
};

struct NodeStmtAssign
{
    Token ident;
    NodeExpr* expr {};
    std::optional<std::string> op_type;
};


struct NodeStmtElse
{
    NodeExpr* expr;
    NodeScope* scope;
};




struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt { 
    std::variant<NodeStmtExit*, NodeStmtLet*,NodeScope*,NodeStmtIf*,NodeStmtElse*,NodeStmtAssign*>var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens))
        ,m_allacator(1024 *1024 *4) // 4mb
    {
    }


    std::optional<NodeTerm*> parse_term()
    {
        if(auto int_lit = try_consume(TokenType::int_lit))
        {
            auto term_int_lit = m_allacator.alloc<NodeTermIntLit>();//temlate give a NodeExprIntLit struct sizeof and included linear memory array
            term_int_lit ->int_lit = int_lit.value();
            auto term = m_allacator.alloc<NodeTerm>();
            term ->var =term_int_lit; 
            return term;
        }


        else if (auto ident = try_consume(TokenType::ident)) {   
            auto term_ident = m_allacator.alloc<NodeTermIdent>();
       
            term_ident ->ident =ident.value();
            
            
            auto term = m_allacator.alloc<NodeTerm>();
            term ->var = term_ident;
            return term;
        
        }
        else if(auto open_paren = try_consume(TokenType::open_paren)){
            auto expr = parse_expr();
            if(!expr.has_value()){
                std::cerr << "Expected expression" << std::endl;
                exit(EXIT_FAILURE);
            } 
            try_consume(TokenType::close_paren,"Expected ')'");
            auto term_paren=m_allacator.alloc<NodeTermParen>();
            term_paren->expr=expr.value();
            auto term=m_allacator.alloc<NodeTerm>(); 
            term->var = term_paren;
            return term;
        }
        else{
            return {};
        }
    }

    std::optional<NodeExpr*> parse_expr(int min_prec=0)
    {
        std::optional<NodeTerm*> term_lhs=parse_term();

        if(!term_lhs.has_value()){
            return {};
        }

        auto expr_lhs=m_allacator.alloc<NodeExpr>();
        expr_lhs->var=term_lhs.value();

        while(true){
            std::optional<Token> current_tok=peek();
            std::optional<int> prec;
            if(current_tok.has_value()){
                prec=bin_prec(current_tok->type);

                if(!prec.has_value() || prec <min_prec){break;}
            }else{
                break;  
            }
            Token op=consume();
            
            int next_min_prec=prec.value()+1;
            auto expr_rhs=parse_expr(next_min_prec);

            if(!expr_rhs.has_value())
            {
                std::cerr << "Unable to parse"<<std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr=m_allacator.alloc<NodeBinExpr>();
            auto expr_lhs2=m_allacator.alloc<NodeExpr>();
                        
            if(op.type == TokenType::plus)
            {
                auto add=m_allacator.alloc<NodeBinExprAdd>();
                expr_lhs2->var = expr_lhs->var;
                add->lhs=expr_lhs2;
                add->rhs=expr_rhs.value();
                expr ->var=add;
            }else if(op.type == TokenType::star)
            {
                auto multi=m_allacator.alloc<NodeBinExprMulti>();
                expr_lhs2->var = expr_lhs->var;
                multi->lhs=expr_lhs2;
                multi->rhs = expr_rhs.value();

                expr ->var=multi;
            }else if(op.type == TokenType::div)
            {
                auto div=m_allacator.alloc<NodeBinExprDiv>();
                div->lhs=expr_lhs2;
                expr_lhs2->var = expr_lhs->var;
                div->rhs = expr_rhs.value();
                expr ->var=div;
            }else if(op.type == TokenType::sub)
            {
                auto sub=m_allacator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs=expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr ->var=sub;
            }
            expr_lhs->var=expr;             
        }
        return expr_lhs;

        
    }

    std::optional<NodeScope*> parse_scope(){
        if(!try_consume(TokenType::open_curly).has_value()){

            return {};
        }
        
        auto scope =m_allacator.alloc<NodeScope>();
        
        while(auto stmt = parse_stmt())
        {
            scope->stmts.push_back(stmt.value());
        }
        try_consume(TokenType::close_curly,"Expected '}'");
        
        return scope;
    }

    std::optional<NodeStmt*> parse_stmt()
    {
        if (peek().value().type == TokenType::exit && peek(1).has_value()
            && peek(1).value().type == TokenType::open_paren) {
            consume();
            consume();
            auto stmt_exit =m_allacator.alloc<NodeStmtExit>();
            if (auto node_expr = parse_expr()) {
                stmt_exit->expr =node_expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren,"Expected ')'");
            try_consume(TokenType::semi,"Expected ';'");
               
            auto stmt = m_allacator.alloc<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        }
        else if (
            peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value()
            && peek(1).value().type == TokenType::ident && peek(2).has_value()
            && peek(2).value().type == TokenType::eq) {
            consume();
            auto stmt_let = m_allacator.alloc<NodeStmtLet>();
            stmt_let->ident =consume();
            consume();
            if (auto expr = parse_expr()) {
                stmt_let->expr = expr.value();
            }
            else {
                std::cerr << "Invalid expression" << std::endl;
             
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::semi,"Expected ';'");
            auto stmt = m_allacator.alloc<NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
        }

        else if(peek().has_value()&& peek().value().type==TokenType::ident && peek(1).has_value() && peek(1).value().type==TokenType::eq){
            const auto assign=m_allacator.alloc<NodeStmtAssign>();
            
            assign->ident=consume();
            consume();
            if(const auto expr=parse_expr())
            {
                assign->expr=expr.value();  
            }else{
                std::cerr <<"Invalid expressionn"<<std::endl;
                exit(EXIT_FAILURE);
            }                
            try_consume(TokenType::semi,"Expected ';' ");

            
            

            auto stmt=m_allacator.emplace<NodeStmt>(assign);
            return stmt;
        }

        else if(peek().has_value() && peek().value().type==TokenType::open_curly){
            if(auto scope=parse_scope()){
                auto stmt=m_allacator.alloc<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            }else{
                std::cerr <<"Invalid scope"<<std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if(auto if_=try_consume(TokenType::_if)){
            try_consume(TokenType::open_paren,"Expected '('");
            auto stmt_if=m_allacator.alloc<NodeStmtIf>();
            if(auto expr=parse_expr()){
                stmt_if->expr=expr.value();
            }else{
                std::cerr << "Invalid expression" << std::endl;
             
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren,"Expected ')'");

            if(auto scope=parse_scope()){
                stmt_if->scope = scope.value();
            }else{
                std::cerr <<"Expected scope" <<std::endl;
                exit(EXIT_FAILURE);
            }    
            auto stmt=m_allacator.alloc<NodeStmt>();
            stmt->var = stmt_if;
            return stmt;

        }

        else if(auto else_=try_consume(TokenType::_else))
        {

            auto stmt_else=m_allacator.alloc<NodeStmtElse>();
            if(auto scope=parse_scope()){
                stmt_else->scope = scope.value();
            }else{
                std::cerr <<"Expected scope" <<std::endl;
                exit(EXIT_FAILURE);
            }    
            auto stmt=m_allacator.alloc<NodeStmt>();
            stmt->var = stmt_else;
            return stmt;
        }


        else {
            return {};
        }
    }

    std::optional<NodeProg> parse_prog()
    {
        NodeProg prog;
        while (peek().has_value()) {
            if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            }
            else {
                std::cerr << "Invalid statement" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return prog;
    }

private:
    [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size()) {
            return {};
        }
        else {
            return m_tokens.at(m_index + offset);
        }
    }

    inline Token consume()
    {
        return m_tokens.at(m_index++);
    }

    inline Token try_consume(TokenType type,const std::string& msg)
    {
        if(peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else{
            std::cerr << msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    inline std::optional<Token> try_consume(TokenType type)
    {
        if(peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else{
            return {};
        }
    }

    inline std::optional<Token> behind(int offset)
    {
        return m_tokens.at(m_index-offset);
    }

    inline std::optional<Token> get_token_index(int index)
    {
        return m_tokens.at(index);
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllacator m_allacator;

};
