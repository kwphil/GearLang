
#include <vector>

#include "lex.cpp"

namespace Ast 
{

    namespace Stat
    {
        class Statement
        {
        public:
            //virtual void generate();
            static Statement parse(Lexer::Stream& s);

        };

        class Expr : public Statement
        {
            enum Type
            {
                Add,
                Sub,
                Mul,
                Div,
                LitInt,
                LitFloat,
                Var,
            } type;

            union
            {
                struct
                {
                    Statement left;
                    Statement right;
                };
                uint64_t uval;
                float fval;
                const char* var;
            };

public:
            static Expr parse(Lexer::Stream& s)
            {
                return parseExpr(s);
            }

            static Expr parseExpr(Lexer::Stream& s)
            {
                auto left = parseTerm(s);
                
                //no operator
                if (s.peek().type != Lexer::Type::Operator)
                    return left;

                Expr out = Expr();
                out.left = left;

                switch(s.pop().content[0])
                {
                    case '+': out.type = Type::Add; break;
                    case '-': out.type = Type::Sub; break;
                    case '*': out.type = Type::Mul; break;
                    case '/': out.type = Type::Div; break;
                }

                out.right = parseTerm(s);
                return out;
            }
            static Expr parseTerm(Lexer::Stream& s)
            {
                if (s.peek().content == "(")
                {
                    s.expect("(");
                    auto expr = parseExpr(s);
                    s.expect(")");
                    return expr;
                }

                Expr out = Expr();
                Lexer::Token lit = s.pop();
                switch (lit.type)
                {
                    case Lexer::Type::FloatLiteral:   out.fval = std::stof(lit.content); break;
                    case Lexer::Type::IntegerLiteral: out.uval = std::stoi(lit.content); break;
                    case Lexer::Type::Identifier:     out.var  = lit.content.c_str();    break;
                }

                return out;
            }


        };


        class Let : public Statement
        {
            
            std::string target;
            Expr expr;
public:
            static Let parse(Lexer::Stream& s)
            {
                Let that = Let();

                s.expect("let");
                that.target = s.pop().content;
                s.expect("=");
                that.expr = Expr::parse(s);

                return that;
            }

        };


        Statement Statement::parse(Lexer::Stream& s)
        {
            Statement out;
            if (s.peek().content == "let") out = Let::parse(s);
            else                           out = Expr::parse(s);

            s.expect(";");

            return out;
        }


    }



    class Program
    {
private:
        std::vector<Stat::Statement> content;
       
public: 
        static Program parse(Lexer::Stream& s)
        {
            Program that = Program();
            while (s.has())
                that.content.push_back(Stat::Statement::parse(s));

            return that;
        }

        
    };




}





