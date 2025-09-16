
#include <vector>
#include <memory>

#include "lex.cpp"

namespace Ast 
{

    namespace Stat
    {
        class Statement
        {
        public:
            virtual ~Statement() = default;
    
            //virtual void generate();
            virtual std::string show() = 0;
            //static Statement parse(Lexer::Stream& s);
        };

        class Expr : public Statement
        {
            enum Type
            {
                Invalid,
                Add,
                Sub,
                Mul,
                Div,
                LitInt,
                LitFloat,
                Var,
            } type;

            struct
            {
                std::unique_ptr<Expr> left;
                std::unique_ptr<Expr> right;
            } branch;
            union
            {
                uint64_t uval;
                float fval;
            } data;
            std::unique_ptr<std::string> var;

public:


            static std::unique_ptr<Expr> parse(Lexer::Stream& s)
            {
                return parseExpr(s);
            }

            static std::unique_ptr<Expr> parseExpr(Lexer::Stream& s)
            {
                auto left = parseTerm(s);
                
                //no operator
                if (s.peek().type != Lexer::Type::Operator)
                    return left;

                Expr out = Expr();

                switch(s.pop().content[0])
                {
                    case '+': out.type = Type::Add; break;
                    case '-': out.type = Type::Sub; break;
                    case '*': out.type = Type::Mul; break;
                    case '/': out.type = Type::Div; break;
                }

                auto right = parseTerm(s);
                return std::make_unique<Expr>(Expr());
            }
            static std::unique_ptr<Expr> parseTerm(Lexer::Stream& s)
            {
                if (s.peek().content == "(")
                {
                    s.expect("(");
                    auto expr = parseExpr(s);
                    s.expect(")");
                    return expr;
                }

                Lexer::Token lit = s.pop();
                double fval;
                uint64_t uval;
                std::string var;
                switch (lit.type)
                {
                    case Lexer::Type::FloatLiteral:   fval = std::stof(lit.content); break;
                    case Lexer::Type::IntegerLiteral: uval = std::stoi(lit.content); break;
                    case Lexer::Type::Identifier:     var  = lit.content.c_str();    break;
                }

                return std::make_unique<Expr>(Expr());
            }

            std::string show() override
            {
                switch(type)
                {
                    case Add: return branch.left->show() + " + " + branch.right->show(); break;
                    case Sub: return branch.left->show() + " - " + branch.right->show(); break;
                    case Mul: return branch.left->show() + " * " + branch.right->show(); break;
                    case Div: return branch.left->show() + " / " + branch.right->show(); break;

                    case LitInt:   return std::to_string(data.uval); break;
                    case LitFloat: return std::to_string(data.fval); break;
                    case Var:      return std::string(*var); break;
                }
            }

        };


        class Let : public Statement
        {
            
            std::unique_ptr<std::string> target;
            std::unique_ptr<Expr> expr;
public:
            static std::unique_ptr<Let> parse(Lexer::Stream& s)
            {

                s.expect("let");
                std::string target = s.pop().content;
                s.expect("=");
                std::unique_ptr<Expr> expr = Expr::parse(s);

                return std::make_unique<Let>(Let());
            }

            std::string show() override
            {
                return "let " + *target + " = " + expr->show() + ";";
            }

        };


        std::unique_ptr<Statement> parseStatement(Lexer::Stream& s)
        {
            std::unique_ptr<Statement> out;

            if (s.peek().content == "let") out = Let::parse(s);
            else                           out = Expr::parse(s);
            s.expect(";");

            return out;
        }


    }



    class Program
    {
private:
        std::vector<std::unique_ptr<Stat::Statement>> content;
       
public: 
        static Program parse(Lexer::Stream& s)
        {
            Program that = Program();
            while (s.has())
                that.content.push_back(Stat::parseStatement(s));

            return that;
        }

        std::string show()
        {
            std::string out;

            for (auto& stat : content)
                out += (stat->show() + "\n");

            return out;
        }

        
    };




}





