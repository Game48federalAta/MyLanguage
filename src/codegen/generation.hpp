#pragma once

#include "../parser/parser.hpp"    
#include <assert.h>
#include <algorithm>
class Generator {
public:
    inline explicit Generator(NodeProg prog)
        : m_prog(std::move(prog))
    {
    }


    void gen_term(const NodeTerm* term,std::string loc)
    {
        struct TermVisitor{
            Generator* gen;
            std::string& loc;
            void operator()(const NodeTermIntLit* term_int_lit)const{
                if(loc.empty()){
                    gen->m_output << "    mov rax, " << term_int_lit ->int_lit.value.value() << "\n";
                    gen->push("rax","");
                }
                else if(loc=="funcs"){
                    gen->m_funcs <<  "    mov rax, " << term_int_lit ->int_lit.value.value() << "\n";
                    gen->push("rax","funcs");
                }
                
            }
            void operator()(const NodeTermIdent* term_ident)const{
                if(loc.empty()){
                    auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(),[&](const Var& var){
                    return var.name==term_ident->ident.value.value();
                    });
            
                    if(it == gen->m_vars.cend()){
                        std::cerr << "Undeclared identifier: "<<term_ident->ident.value.value()<<std::endl;
                        exit(EXIT_FAILURE);
                    }
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen->m_stack_size - (*it).stack_loc - 1) * 8 << "]";
                    gen->push(offset.str(),"");
                }
                else if(loc=="funcs")
                {
                    auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(),[&](const Var& var){
                    return var.name==term_ident->ident.value.value();
                    });
            
                    if(it == gen->m_vars.cend()){
                        std::cerr << "Undeclared identifier: "<<term_ident->ident.value.value()<<std::endl;
                        exit(EXIT_FAILURE);
                    }
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen->m_stack_size - (*it).stack_loc - 1) * 8 << "]";
                    gen->push(offset.str(),"funcs");
                }
            }
            void operator()(const NodeTermParen* paren)
            {
                if(loc.empty()){
                    gen->gen_expr(paren->expr,"");    
                }else if(loc=="funcs"){
                    gen->gen_expr(paren->expr,"funcs");
                }
            }
            void operator()(const NodeTermRetVal* ret){
                gen->m_funcs << "    mov rax, "<<ret->int_lit.value.value()<<"\n";
                gen->m_funcs<<"    ret\n";
            }
            
        };

        TermVisitor visitor{.gen = this,.loc=loc};
        std::visit(visitor,term->var);
    }

    void gen_bin_expr(const NodeBinExpr* bin_expr,std::string loc){
        struct BinExprVisitor{
            Generator* gen;
            std::string& loc;
            void operator()(const NodeBinExprSub* sub)
            {
                if(loc.empty())
                {
                    gen->gen_expr(sub->rhs,"") ;
                    gen->gen_expr(sub->lhs,"");
                    gen->pop("rax","");
                    gen->pop("rbx","");
                    gen->m_output << "    sub rax,rbx\n";
                    gen->push("rax","");
                }
                else if(loc=="funcs"){
                    gen->gen_expr(sub->rhs,"funcs") ;
                    gen->gen_expr(sub->lhs,"funcs");
                    gen->pop("rax","funcs");
                    gen->pop("rbx","funcs");
                    gen->m_funcs << "    sub rax,rbx\n";
                    gen->push("rax","funcs");
                }
            }
            void operator()(const NodeBinExprAdd* add)
            {
                if(loc.empty()){
                    gen->gen_expr(add->rhs,"") ;
                    gen->gen_expr(add->lhs,"");
                    gen->pop("rax","");
                    gen->pop("rbx","");
                    gen->m_output << "    add rax,rbx\n";
                    gen->push("rax","");
                }
                else if(loc=="funcs"){
                    gen->gen_expr(add->rhs,"funcs") ;
                    gen->gen_expr(add->lhs,"funcs");
                    gen->pop("rax","funcs");
                    gen->pop("rbx","funcs");
                    gen->m_funcs << "    add rax,rbx\n";
                    gen->push("rax","funcs");
                }

            }
            void operator()(const NodeBinExprMulti* multi){
                if(loc.empty()){
                    gen->gen_expr(multi->rhs,"") ;
                    gen->gen_expr(multi->lhs,"");
                    gen->pop("rax","");
                    gen->pop("rbx","");
                    gen->m_output << "    mul rbx\n";
                    gen->push("rax","");
                }else if(loc=="funcs"){
                    gen->gen_expr(multi->rhs,"funcs") ;
                    gen->gen_expr(multi->lhs,"funcs");
                    gen->pop("rax","funcs");
                    gen->pop("rbx","funcs");
                    gen->m_funcs << "    mul rbx\n";
                    gen->push("rax","funcs");
                }
            }

            void operator()(const NodeBinExprDiv* div)
            {
                if(loc.empty()){
                    gen->gen_expr(div->rhs,"");
                    gen->gen_expr(div->lhs,"");
                    gen->pop("rax","");
                    gen->pop("rbx","");
                    gen->m_output << "    div rbx\n";
                    gen->push("rax","");
                }
                else if(loc=="funcs"){
                    gen->gen_expr(div->rhs,"funcs");
                    gen->gen_expr(div->lhs,"funcs");
                    gen->pop("rax","funcs");
                    gen->pop("rbx","funcs");
                    gen->m_funcs << "    div rbx\n";
                    gen->push("rax","funcs");
                }
            }
            
        };

        BinExprVisitor visitor{.gen=this,.loc=loc};
        std::visit(visitor,bin_expr->var);
    }

    void gen_expr(const NodeExpr* expr,std::string loc)
    {
        struct ExprVisitor {
            Generator* gen;
            std::string& loc;
            void operator()(const NodeTerm* term) const
            {
                gen->gen_term(term,loc);
            }
            void operator()(const NodeBinExpr* bin_expr) const
            {
                gen->gen_bin_expr(bin_expr,loc);
            }
        };

        ExprVisitor visitor { .gen = this,.loc=loc};
        std::visit(visitor, expr -> var);
    }

    void gen_scope(const NodeScope* scope, std::string loc){
        begin_scope();
        for(const NodeStmt* stmt: scope->stmts){
            gen_stmt(stmt,loc);
        }
        end_scope(loc);
    }

    void gen_stmt(const NodeStmt* stmt,std::string loc)
    {
        struct StmtVisitor {
            Generator* gen;
            std::string loc;
            void operator()(const NodeStmtExit* stmt_exit) const
            {
                if(loc.empty()){
                    gen->gen_expr(stmt_exit ->expr,"");  
                    gen->m_output << "    mov rax, 60\n";
                    gen->pop("rdi","");
                    gen->m_output << "    syscall\n";
                }
                else if(loc=="funcs"){
                    std::cout <<"test"<<std::endl;
                    gen->gen_expr(stmt_exit ->expr,"funcs");  
                    gen->m_funcs << "    mov rax, 60\n";
                    gen->pop("rdi","func");
                    gen->m_funcs << "    syscall\n";
                }
            }
            void operator()(const NodeStmtLet* stmt_let) const
            {
                if(loc.empty()){
                    auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(),[&](const Var& var){
                    return var.name==stmt_let->ident.value.value();
                    });
                    if (it != gen->m_vars.cend()) {
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                    }
                    gen->m_vars.push_back({ .name = stmt_let->ident.value.value(),.stack_loc = gen->m_stack_size });
                    gen->gen_expr(stmt_let->expr,"");
                }
                else if(loc=="funcs"){
                    auto it = std::find_if(gen->m_vars.cbegin(),gen->m_vars.cend(),[&](const Var& var){
                    return var.name==stmt_let->ident.value.value();
                    });
                    if (it != gen->m_vars.cend()) {
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                    }
                    gen->m_vars.push_back({ .name = stmt_let->ident.value.value(),.stack_loc = gen->m_stack_size });
                    gen->gen_expr(stmt_let->expr,"funcs");
                }
                
            }
            void operator()(const NodeScope * scope)const{
                gen->gen_scope(scope,loc);
            }

            void operator()(const NodeStmtAssign* stmt_assign)const{
                auto it=std::ranges::find_if(gen->m_vars,[&](const Var& var){
                    return var.name == stmt_assign->ident.value.value();    
                }); 

                if(it == gen->m_vars.end())
                {
                    std::cerr <<"undeclared idendifier"<<std::endl;
                    exit(EXIT_FAILURE);

                }
                if(stmt_assign->op_type=="+"){
                    std::cout <<"OL"<<std::endl;  
                    gen->gen_expr(stmt_assign->expr,loc);
                    if(loc.empty())
                    {
                        gen->pop("rax","");
                        gen->m_output<<"	add "<<"[rsp + "<<(gen->m_stack_size - it->stack_loc -1)*8 <<"], rax\n";
                    }
                    else if(loc=="funcs"){
                        gen->pop("rax","funcs");
                        gen->m_funcs<<"	add "<<"[rsp + "<<(gen->m_stack_size - it->stack_loc -1)*8 <<"], rax\n";
                    }
                
                }else if(stmt_assign->op_type=="*"){
                    std::cout <<"lL"<<std::endl;

                    
                    gen->gen_expr(stmt_assign->expr,loc ) ;
                    if(loc.empty()){
                        gen->pop("rax","");
                        gen->pop("rbx","");
                        gen->m_output << "    mul rbx\n";
                        gen->push("rax","");
                    }else if(loc=="funcs")
                    {
                        gen->pop("rax","funcs");
                        gen->pop("rbx","funcs");
                        gen->m_funcs << "    mul rbx\n";
                        gen->push("rax","funcs");

                    }
                
                    

                }else if(stmt_assign->op_type =="/"){
                    // TODO fix to dividing in assigment
                    //cause cannot get value of ident
                    /*gen->gen_expr(stmt_assign->expr) ;
                    gen->pop("rax");
                    gen->pop("rbx");
                    gen->m_output << "    div rbx\n";
                    gen->push("rax");*/

                    std::cerr << "Now cannot divide in assigment try {x}={x}/{value}" << std::endl;
                    exit(EXIT_FAILURE);
                }
                else if(stmt_assign->op_type=="-"){
                    if(loc.empty()){
                        gen->gen_expr(stmt_assign->expr,"");
                        gen->pop("rax","");
                        gen->m_output<<"	sub "<<"[rsp + "<<(gen->m_stack_size - it->stack_loc -1)*8 <<"], rax\n";
                    
                    }
                    else if(loc=="funcs"){
                        gen->gen_expr(stmt_assign->expr,"funcs");
                        gen->pop("rax","funcs");
                        gen->m_funcs<<"	sub "<<"[rsp + "<<(gen->m_stack_size - it->stack_loc -1)*8 <<"], rax\n";    
                    }

                    std::cout <<"OL"<<std::endl;  
                    
                }else{
                    if(loc.empty()){
                        gen->gen_expr(stmt_assign->expr,"");
                        gen->pop("rax","");
                        gen->m_output<<"   mov [rsp +" << (gen->m_stack_size - it->stack_loc -1)*8 <<"], rax\n";
                    }
                    else if(loc=="funcs"){
                        gen->gen_expr(stmt_assign->expr,"funcs");
                        gen->pop("rax","funcs");
                        gen->m_funcs<<"   mov [rsp +" << (gen->m_stack_size - it->stack_loc -1)*8 <<"], rax\n";
                    }
                }
                
            }

            void operator()(const NodeStmtIf* stmt_if)const{

                if(loc.empty())
                {
                    gen->gen_expr(stmt_if->expr,"");
                    gen->pop("rax","");
                    std::string label_=gen->create_label();
                    gen-> m_output <<"    test rax, rax\n";
                    gen->m_output << "    jz "<<label_<<"\n"; // jz mean if expr if statements equal 0 jump other label well is passing if statement if not have else statement
                    gen->gen_scope(stmt_if->scope,"");
                    gen->m_output <<label_ << ":\n";
                }
                else if(loc=="funcs"){
                    gen->gen_expr(stmt_if->expr,"funcs");
                    gen->pop("rax","funcs");
                    std::string label_=gen->create_label();
                    gen-> m_funcs <<"    test rax, rax\n";
                    gen->m_funcs << "    jz "<<label_<<"\n"; // jz mean if expr if statements equal 0 jump other label well is passing if statement if not have else statement
                    gen->gen_scope(stmt_if->scope,"funcs");
                    gen->m_funcs <<label_ << ":\n";
                }
            }

            void operator()(const NodeStmtElse* stmt_else)const{
                if(loc.empty())
                {
                    std::string label_=gen->create_label();

                    gen->gen_scope(stmt_else->scope,"");
                    gen->m_output << "    jmp "<<label_<<"\n";
                    gen->m_output <<label_ << ":\n";
                }else if(loc=="funcs"){
                    std::string label_=gen->create_label();

                    gen->gen_scope(stmt_else->scope,"funcs");
                    gen->m_funcs << "    jmp "<<label_<<"\n";
                    gen->m_funcs <<label_ << ":\n";
                }
                
                
            }
            void operator()(const NodeStmtFunc* func) const{
                std::cout << func->ident.value.value()<<std::endl;
                std::string label=gen->create_label(func->ident.value.value());
                gen->m_funcs <<label << ":\n";
                gen->gen_scope(func->scope,"funcs");
            }
            void operator()(const NodeStmtCall* call)const{
                std::string label=gen->create_label(call->ident.value.value());
                gen->m_output <<"    call "<<label <<"\n";
            }
            void operator()(const NodeStmtRetVal* ret)const{
                gen->gen_expr(ret->expr,"funcs");
                //gen->m_output<<"    ret\n";
                std::cout <<" selam111"<<std::endl;
            }
            
            

        };

        StmtVisitor visitor { .gen = this, .loc = loc};
        std::visit(visitor, stmt->var);
    }



    [[nodiscard]] std::string gen_prog()
    {
        m_output <<"section .text\n ";
        m_output << "    global _start\n_start:\n";
        std::cout << "ss"<<std::endl;
        for (const NodeStmt* stmt : m_prog.stmts) {
            gen_stmt(stmt,"");
        }
        
        m_output<<"    mov rdi,rax\n";
        m_output<<"    mov rax, 60\n";
        m_output<<"    syscall\n";

        m_output<<m_funcs.str();
        return m_output.str();
    }

private:
    void push(const std::string& reg, std::string loc)
    {
        if(loc =="funcs"){
            m_funcs <<"    push "<<reg<<"\n";
            m_stack_size++;
        }else if(loc.empty()){
            m_output << "    push " << reg << "\n";
            m_stack_size++;
        }
    }

    void pop(const std::string& reg,std::string loc)
    {
        if(loc=="funcs"){
            m_funcs << "    pop "<<reg<<"\n";
            m_stack_size--;
        }else if(loc.empty()){
            m_output << "    pop " << reg << "\n";
            m_stack_size--;
        }
    }

    void begin_scope(){
        m_scopes.push_back(m_vars.size());
    }

    void end_scope(std::string loc){
        if(loc=="funcs"){
            size_t pop_count = m_vars.size() - m_scopes.back();
            m_funcs << "    add rsp, " << pop_count * 8  <<"\n";
            m_stack_size -= pop_count;

            for(int i = 0; i < pop_count ; i++){
                m_vars.pop_back();
            }
            m_scopes.pop_back();
        }else if(loc.empty()){
            size_t pop_count = m_vars.size() - m_scopes.back();
            m_output << "    add rsp, " << pop_count * 8  <<"\n";
            m_stack_size -= pop_count;

            for(int i = 0; i < pop_count ; i++){
                m_vars.pop_back();
            }
            m_scopes.pop_back();
        }
    }
    
    
    std::string create_label(){
        std::stringstream label;

        label << "label"<< m_label_count++;
        return label.str();
        
    }
    std::string create_label(std::string name){
        std::stringstream label;
        label << name;
        return label.str();
    }

    struct Var {
        std::string name;
        size_t stack_loc;
    };

    const NodeProg m_prog;
    std::stringstream m_output;

    std::stringstream main_;
    std::stringstream m_funcs; 

    size_t m_stack_size = 0;
    std::vector<Var> m_vars {};
    std::vector<size_t>m_scopes {};
    int m_label_count=0;



};
