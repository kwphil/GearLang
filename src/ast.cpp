
#include <vector>
#include <memory>

#include "lex.cpp"
#include "ctx.cpp"

namespace Ast 
{

    namespace Nodes
    {
        class NodeBase
        {
        public:
            virtual ~NodeBase() = default;
    
            virtual std::string show() = 0;
            virtual void generate(Context& ctx) = 0;
            static std::unique_ptr<NodeBase> parse(Lexer::Stream& s);
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
                    case Add: return "(" + left->show() + " + " + right->show() + ")"; break;
                    case Sub: return "(" + left->show() + " - " + right->show() + ")"; break;
                    case Mul: return "(" + left->show() + " * " + right->show() + ")"; break;
                    case Div: return "(" + left->show() + " / " + right->show() + ")"; break;
                }
            }

            void generate(Context& ctx) override 
            {
                right->generate(ctx);
                ctx.emit("push rax");
                left->generate(ctx);
                ctx.emit("pop rbx");
                
                switch (type)
                {
                    case Add: ctx.emit("add rax, rbx"); break;
                    case Sub: ctx.emit("sub rax, rbx"); break;
                    case Mul: ctx.emit("imul rbx"); break;
                    case Div: ctx.emit("idiv rbx"); break;
                }


            };
        };

        class ExprLitInt : public Expr
        {
            uint64_t val;
            
        public:
            ExprLitInt(uint64_t x) : val(x) {}
            static std::unique_ptr<ExprLitInt> parse(Lexer::Stream& s) 
            { return std::make_unique<ExprLitInt>((uint64_t)std::stoi(s.pop().content)); }
            std::string show() override { return std::to_string(val); }

            void generate(Context& ctx) override 
            {
                ctx.emit("mov rax, " + std::to_string(val));
            };
        };

        class ExprLitFloat : public Expr
        {
            double val;

        public:
            ExprLitFloat(double x) : val(x) {}
            static std::unique_ptr<ExprLitFloat> parse(Lexer::Stream& s) 
            { return std::make_unique<ExprLitFloat>(std::stod(s.pop().content)); }
            std::string show() override { return std::to_string(val); }
            
            void generate(Context& ctx) override {}; //TODO
        };

        class ExprVar : public Expr
        {
            std::unique_ptr<std::string> name;

        public:
            ExprVar(std::unique_ptr<std::string> name)
            : name(std::move(name)) {};
            static std::unique_ptr<ExprVar> parse(Lexer::Stream& s)
            { 
                return std::make_unique<ExprVar>(
                    std::make_unique<std::string>(s.pop().content)
                ); 
            }

            std::string show() override { return *name; }
            void generate(Context& ctx) override 
            {
                uint64_t var_addr = ctx.var(*name) * 8;
                ctx.emit("mov rax, [vars + " + std::to_string(var_addr) + "]");
            };
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

            void generate(Context& ctx) override
            {
                uint64_t var_addr = ctx.var(*target) * 8;
                
                expr->generate(ctx);
                ctx.emit("mov [vars + " + std::to_string(var_addr) + "], rax");
            }

        };


        std::unique_ptr<NodeBase> NodeBase::parse(Lexer::Stream& s)
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
                that.content.push_back(Nodes::NodeBase::parse(s));

            return that;
        }

        std::string show()
        {
            std::string out;

            for (auto& node : content)
                out += (node->show() + "\n");

            return out;
        }

        void generate(Context& ctx)
        {
            for (const auto& expr : content)
                expr->generate(ctx);
        }

        
    };




}





