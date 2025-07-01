#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

class Value
{
        friend std::ostream& operator<< (std::ostream &os, Value const &value);

        public:
                Value(float data, std::string label, std::set<Value*> children = {}, char op = 0x0) :
                        data_{data}, grad_{0.0}, label_{std::move(label)}, children_{std::move(children)}, op_{op}
                {
                }

                bool operator<(Value const &other) const
                {
                        return  this < &other;
                }

                Value operator+(Value &other)
                {
                        char op = '+';
                        return Value{this->data_ + other.data_, std::string{this->label_ + op + other.label_}, {this, &other}, op};
                }

                Value operator*(Value &other)
                {
                        char op = '*';
                        return Value{this->data_ * other.data_, std::string{this->label_ + op + other.label_}, {this, &other}, op};
                }

                float getData() const
                {
                        return data_;
                }

                float getGrad() const
                {
                        return grad_;
                }

                void setLabel(std::string label)
                {
                        label_ = std::move(label);
                }

                std::string_view getLabel() const
                {
                        return label_;
                }

                auto& getChildren() const
                {
                        return children_;
                }

                std::string id() const
                {
                        std::stringstream ss;
                        ss << "Node" << reinterpret_cast<uintptr_t>(this);
                        return ss.str();
                }

                bool hasOp() const
                {
                        return op_ != 0x00;
                }

                char getOp() const
                {
                        return op_;
                }
        private:
                float data_;
                float grad_;
                std::string label_;
                std::set<Value*> children_;
                char op_;
};

std::ostream& operator <<(std::ostream &os, Value const &value)
{

        os << "Value(";

        //constexpr auto ptrmask = 0xfffff;
        //os << std::hex << std::showbase;
        //auto addr = reinterpret_cast<uintptr_t>(&value);
        //os << (addr & ptrmask) << '|';

        os << value.label_ << '|';

        os << std::dec << std::fixed << std::setprecision(5);
        os << value.data_;

        if (value.op_)
        {
                os << '|' << value.op_;
        }

        bool first = true;
        for (auto const &child : value.children_)
        {
                if (first) {
                        os << '|';
                        first = false;
                }
                else {
                        os << ' ';
                }
                auto childaddr = reinterpret_cast<uintptr_t>(child);
                os << std::hex << std::showbase;
                //os << (childaddr & ptrmask);
                os << child->label_;
        }

        os << ')';

        return os;
}

namespace dot {

void writeNodes(const Value& node, std::ofstream& of)
{
        of << "        " << node.id() << " [label=\"" << node.getLabel() << "|data:" << std::dec << std::fixed << std::setprecision(4) << node.getData() << "|grad:" << node.getGrad() << "\", shape=\"record\"];\n";
        if (node.hasOp())
        {
                of << "        " << node.id() << "_Op" << " [label=\"" << node.getOp() << "\"];\n";
        }

        for (const auto* child : node.getChildren())
        {
                writeNodes(*child, of);
        }
}

void writeEdges(const Value& node, std::ofstream& of)
{
        for (const auto* child : node.getChildren())
        {
                of << "        " << child->id() << " -> ";

                of << node.id();

                if (node.hasOp())
                {
                        of << "_Op";
                }

               of << '\n';      
        }

        if (node.hasOp())
        {
                of << "        " << node.id() << "_Op" << " -> " << node.id() << '\n';
        }

        if (!node.getChildren().empty())
        {
                of << '\n';
        }

        for (const auto* child : node.getChildren())
        {
                writeEdges(*child, of);
        }
}

void draw(const Value& root, const char* filename)
{
        std::ofstream of{filename};

        of << "digraph G {\n";
        
        writeNodes(root, of);

        of << '\n';

        writeEdges(root, of);

        of << "}\n";
}

} // namespace dot

int main(int argc, char* argv[])
{
        float h = 0.001;

        float L1, L2;

        {
            Value a{float{2.0} + h, "a"};
            Value b{-3.0, "b"};
            Value c{10.0, "c"};
            auto e = a * b; e.setLabel("e");
            auto d = e + c; d.setLabel("d");
            Value f{-2, "f"};
            auto L = d * f; L.setLabel("L");
            L1 = L.getData();
        }

        {
            Value a{float{2.0}, "a"};
            Value b{-3.0, "b"};
            Value c{10.0, "c"};
            auto e = a * b; e.setLabel("e");
            auto d = e + c; d.setLabel("d");
            Value f{-2, "f"};
            auto L = d * f; L.setLabel("L");
            L2 = L.getData();
        }

        std::cout << "L1: " << L1 << '\n';
        std::cout << "L2: " << L2 << '\n';
        std::cout << "L1-L2: " << L1-L2 << '\n';
        std::cout << "(L1-L2)/h: " << (L1-L2)/h << '\n';

        //dot::draw(L, "L_graph.dot");

        return 0;
}
