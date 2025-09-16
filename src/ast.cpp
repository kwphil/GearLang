
#include <vector>
#include <memory>

#include "lex.cpp"

namespace Ast 
{

    namespace Nodes
    {
        class NodeBase
        {
        public:
            virtual ~NodeBase() = default;
    
            //virtual void generate();
            virtual std::string show() = 0;
            //static Statement parse(Lexer::Stream& s);
        };


        class Expr : public NodeBase
        {
        public:
            virtual ~Expr() = default;

            static std::unique_ptr<Expr> parse(Lexer::Stream& s)
            {
                return parseExpr(s);
            }

            static std::unique_ptr<Expr> parseExpr(Lexer::Stream& s);
            static std::unique_ptr<Expr> parseTerm(Lexer::Stream& s);


        };
        using pExpr = std::unique_ptr<Expr>;




        class ExprOp : public Expr
        {
        public:
            enum Type { Add, Sub, Mul, Div } type;
            std::unique_ptr<Expr> left, right;

            ExprOp(Type type, pExpr left, pExpr right)
            : type(type), left(std::move(left)), right(std::move(right)) {};

            std::string show() override
            {
                switch (type)
                {
                    case Add: return left->show() + " + " + right->show(); break;
                    case Sub: return left->show() + " - " + right->show(); break;
                    case Mul: return left->show() + " * " + right->show(); break;
                    case Div: return left->show() + " / " + right->show(); break;
                }
            }

        };

        class ExprLitInt : public Expr
        {
            uint64_t val;
            
        public:
            ExprLitInt(uint64_t x) : val(x) {}
            static std::unique_ptr<ExprLitInt> parse(Lexer::Stream s) 
            { return std::make_unique<ExprLitInt>((uint64_t)std::stoi(s.pop().content)); }
            std::string show() override { return std::to_string(val); }
        };

        class ExprLitFloat : public Expr
        {
            double val;

        public:
            ExprLitFloat(double x) : val(x) {}
            static std::unique_ptr<ExprLitFloat> parse(Lexer::Stream s) 
            { return std::make_unique<ExprLitFloat>(std::stod(s.pop().content)); }
            std::string show() override { return std::to_string(val); }
        };

        class ExprVar : public Expr
        {
            std::unique_ptr<std::string> name;

        public:
            ExprVar(std::unique_ptr<std::string> name)
            : name(std::move(name)) {};
            static std::unique_ptr<ExprVar> parse(Lexer::Stream s)
            { 
                return std::make_unique<ExprVar>(
                    std::make_unique<std::string>(s.pop().content)
                ); 
            }

            std::string show() override { return *name; }
        };


        pExpr Expr::parseExpr(Lexer::Stream& s)
        {
            pExpr left = parseTerm(s);
            
            //no operator
            if (s.peek().type != Lexer::Type::Operator)
                return left;

            
            ExprOp::Type type;
            switch(s.pop().content[0])
            {
                case '+': type = ExprOp::Type::Add; break;
                case '-': type = ExprOp::Type::Sub; break;
                case '*': type = ExprOp::Type::Mul; break;
                case '/': type = ExprOp::Type::Div; break;
            }

            pExpr right = parseTerm(s);
            return std::make_unique<ExprOp>(type, std::move(left), std::move(right));
        }



        pExpr Expr::parseTerm(Lexer::Stream& s)
        {
            if (s.peek().content == "(")
            {
                s.expect("(");
                auto expr = parseExpr(s);
                s.expect(")");
                return expr;
            }

            Lexer::Token lit = s.peek();
            switch (lit.type)
            {
                case Lexer::Type::FloatLiteral:   return ExprLitFloat::parse(s); break;
                case Lexer::Type::IntegerLiteral: return ExprLitInt::parse(s);   break;
                case Lexer::Type::Identifier:     return ExprVar::parse(s);      break;
            }
        }




        class Let : public NodeBase
        {
            
            std::unique_ptr<std::string> target;
            pExpr expr;
public:
            Let(std::unique_ptr<std::string> target, pExpr expr) 
            : target(std::move(target)), expr(std::move(expr)) {}

            static std::unique_ptr<Let> parse(Lexer::Stream& s)
            {

                s.expect("let");
                std::string target = s.pop().content;
                s.expect("=");
                std::unique_ptr<Expr> expr = Expr::parse(s);

                return std::make_unique<Let>(std::make_unique<std::string>(target), std::move(expr));
            }

            std::string show() override
            {
                return "let " + *target + " = " + expr->show() + ";";
            }

        };


        std::unique_ptr<NodeBase> parseStatement(Lexer::Stream& s)
        {
            std::unique_ptr<NodeBase> out;

            if (s.peek().content == "let") out = Let::parse(s);
            else                           out = Expr::parse(s);
            s.expect(";");

            return out;
        }


    }


    class Program
    {
private:
        std::vector<std::unique_ptr<Nodes::NodeBase>> content;
       
public: 
        static Program parse(Lexer::Stream& s)
        {
            Program that = Program();
            while (s.has())
                that.content.push_back(Nodes::parseStatement(s));

            return that;
        }

        std::string show()
        {
            std::string out;

            for (auto& node : content)
                out += (node->show() + "\n");

            return out;
        }

        
    };




}





