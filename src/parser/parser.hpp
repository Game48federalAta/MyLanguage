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

struct NodeBinExprAdd
{
    NodeExpr* lhs;//left hand size
    NodeExpr* rhs;//right hand size 
};

/*struct BinExprMulti
{
    NodeExpr* lhs;//left hand size
    NodeExpr* rhs;//right hand size 
};*/


struct NodeBinExpr{
    NodeBinExprAdd* add;
};




struct NodeTerm
{
    std::variant<NodeTermIntLit*,NodeTermIdent*> var;
};


struct NodeExpr {
    std::variant<NodeTerm*,NodeBinExpr*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*> var;
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
        else{
            return {};
        }
    }

    std::optional<NodeExpr*> parse_expr()
    {

        if(auto term = parse_term())
        {

            if(try_consume(TokenType::plus).has_value())
            {
                auto bin_expr=m_allacator.alloc<NodeBinExpr>();
                
                auto bin_expr_add=m_allacator.alloc<NodeBinExprAdd>();
                auto lhs_expr = m_allacator.alloc<NodeExpr>();
                lhs_expr -> var =term.value();
                bin_expr_add ->lhs = lhs_expr;   
                if(auto rhs = parse_expr())
                { 
                    bin_expr_add ->rhs = rhs.value();
                    bin_expr ->add = bin_expr_add;
                    auto expr=m_allacator.alloc<NodeExpr>();
                    expr ->var = bin_expr;
                    return expr;
                }
                else{
                    std::cerr << "Expected expressino"<< std::endl;
                    exit(EXIT_FAILURE);
                }

            }else{
                auto expr = m_allacator.alloc<NodeExpr>();
                
                expr->var = term.value();
                return expr;
                
            }
        }else{
            return {};  
        }
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

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllacator m_allacator;

};