#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

class Value
{
    friend std::ostream &operator<<(std::ostream &os, Value const &value);

    friend Value operator+(Value &first, float second);
    friend Value operator+(float first, Value &second);

    friend Value operator*(Value &first, float second);
    friend Value operator*(float first, Value &second);

    friend void backprop(Value &);

  public:
    Value(float data = 0.0, std::string label = {}, std::array<Value *, 2> children = {nullptr, nullptr}, std::string op = {})
        : data_{data}, grad_{0.0}, label_{label}, children_{std::move(children)}, op_{op},
          backwards_{[/*this*/](Value& out) { /*std::cout << "Backwards terminal: " << label_ << '\n';*/ }}
    {
    }

    bool operator<(Value const &other) const
    {
        return this < &other;
    }

    Value operator+(Value &other)
    {
        std::string op{'+'};

        auto out = Value{this->data_ + other.data_, std::string{this->label_ + op + other.label_}, {this, &other}, op};

        auto backwards = [](Value& out) {
            assert(out.children_[0] && out.children_[1]);

            //std::cout << "Backwards +: " << out.label_ << '\n';

            out.children_[0]->grad_ += out.grad_;
            out.children_[1]->grad_ += out.grad_;

            out.children_[0]->backwards();
            out.children_[1]->backwards();
        };

        out.setBackwards(std::move(backwards));

        return out;
    }

    Value operator*(Value &other)
    {
        std::string op{'*'};

        auto out = Value{this->data_ * other.data_, std::string{this->label_ + op + other.label_}, {this, &other}, op};

        auto backwards = [](Value& out) {
            assert(out.children_[0] && out.children_[1]);

            //std::cout << "Backwards *: " << out.label_ << '\n';

            out.children_[0]->grad_ += out.children_[1]->data_ * out.grad_;
            out.children_[1]->grad_ += out.children_[0]->data_ * out.grad_;

            out.children_[0]->backwards();
            out.children_[1]->backwards();
        };

        out.setBackwards(std::move(backwards));

        return out;
    }

    Value pow(float other)
    {
        std::string op{"pow"};

        auto out = Value{std::pow(this->data_, other),
                         std::string{op + '(' + this->label_ + ',' + std::to_string(other) + ')'},
                         {this, nullptr},
                         op};

        //TODO: not sure if this is correct, but unused for now
        auto backwards = [other](Value& out) {
            assert(out.children_[0]);

            //std::cout << "Backwards pow(" << other << "): " << out.label_ << '\n';

            out.children_[0]->grad_ += other * std::pow(out.data_, other - 1) * out.grad_;

            out.children_[0]->backwards();
        };

        out.setBackwards(std::move(backwards));

        return out;
    }

    Value tanh()
    {
        std::string op{"tanh"};
        auto out =
            Value{std::tanh(this->data_), std::string{"tanh("} + this->label_ + std::string{")"}, {this, nullptr}, op};

        auto backwards = [](Value& out) {
            assert(out.children_[0]);

            //std::cout << "Backwards tanh: " << out.label_ << '\n';

            out.children_[0]->grad_ += (1 - (out.data_ * out.data_)) * out.grad_;

            out.children_[0]->backwards();
        };

        out.setBackwards(std::move(backwards));

        return out;
    }

    Value exp()
    {
        std::string op{"exp"};
        auto out =
            Value{std::exp(this->data_), std::string("exp(" + this->label_ + std::string{")"}), {this, nullptr}, op};

        auto backwards = [](Value& out) {
            assert(out.children_[0]);

            //std::cout << "Backwards exp: " << out.label_ << '\n';

            out.children_[0]->grad_ += out.data_ * out.grad_;

            out.children_[0]->backwards();
        };

        out.setBackwards(std::move(backwards));

        return out;
    }

    void setBackwards(std::function<void(Value& out)> func)
    {
        backwards_ = std::move(func);
    }

    void backwards()
    {
        backwards_(*this);
    }

    float getData() const
    {
        return data_;
    }

    void setData(float data)
    {
        data_ = data;
    }

    float getGrad() const
    {
        return grad_;
    }

    void resetGrad()
    {
        grad_ = 0.0;
        if (children_[0])
        {
            children_[0]->resetGrad();
        }
        if (children_[1])
        {
            children_[1]->resetGrad();
        }
    }

    void setLabel(std::string label)
    {
        label_ = std::move(label);
    }

    std::string_view getLabel() const
    {
        return label_;
    }

    auto &getChildren() const
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
        return !op_.empty();
    }

    std::string getOp() const
    {
        return op_;
    }

  private:
    float data_;
    float grad_;
    std::array<Value *, 2> children_;
    std::function<void(Value& out)> backwards_;
    std::string label_;
    std::string op_;
};

Value operator+(Value &first, float second)
{
    std::string op = "+";
    std::stringstream ss;
    ss << op << second;

    auto out =
        Value{first.data_ + second, std::string{first.label_ + op + std::to_string(second)}, {&first, nullptr}, op};

    auto backwards = [](Value& out) {
        assert(out.children_[0]);

        // std::cout << "Backwards +: " << out.label_ << '\n';
        out.children_[0]->grad_ += out.grad_;

        out.children_[0]->backwards();
    };

    out.setBackwards(std::move(backwards));

    return out;
}

Value operator+(float first, Value &second)
{
    return second + first;
}

Value operator*(Value &first, float second)
{
    std::string op{'*'};

    auto out =
        Value{first.data_ * second, std::string{first.label_ + op + std::to_string(second)}, {&first, nullptr}, op};

    auto backwards = [second](Value& out) {
        assert(out.children_[0]);

        // std::cout << "Backwards " << op << ": " << out.label_ << '\n';

        out.children_[0]->grad_ += second * out.grad_;

        out.children_[0]->backwards();
    };

    out.setBackwards(std::move(backwards));

    return out;
}

Value operator*(float first, Value &second)
{
    return second * first;
}

std::ostream &operator<<(std::ostream &os, Value const &value)
{

    os << "Value(";

    // constexpr auto ptrmask = 0xfffff;
    // os << std::hex << std::showbase;
    // auto addr = reinterpret_cast<uintptr_t>(&value);
    // os << (addr & ptrmask) << '|';

    os << value.label_ << '|';

    os << std::dec << std::fixed << std::setprecision(5);
    os << value.data_;

    if (!value.op_.empty())
    {
        os << '|' << value.op_;
    }

    bool first = true;
    for (auto const *child : value.children_)
    {
        if (!child)
        {
            continue;
        }

        if (first)
        {
            os << '|';
            first = false;
        }
        else
        {
            os << ' ';
        }
        auto childaddr = reinterpret_cast<uintptr_t>(child);
        os << std::hex << std::showbase;
        // os << (childaddr & ptrmask);
        os << child->label_;
    }

    os << ')';

    return os;
}

void backprop(Value &root)
{
    root.resetGrad();
    root.grad_ = 1.0;
    root.backwards();
}

template<unsigned Size>
class Neuron
{
    public:
        Neuron(float bias) : weights_{}, products_{}, sums_{}, bias_{bias, "bias"}
        {
            for (unsigned i = 0; i < Size; ++i)
            {
                weights_[i] = {0.0, "w[" + std::to_string(i) + ']'};
            }
        }

        Value operator()(std::vector<Value>& inputs)
        {
            assert (inputs.size() == Size);

            for (unsigned i = 0; i < Size; ++i)
            {
                products_[i] = weights_[i] * inputs[i]; products_[i].setLabel("products_[" + std::to_string(i) + ']');
                sums_[i] = products_[i] + (i > 0 ? sums_[i-1] : bias_); sums_[i].setLabel("sums_[" + std::to_string(i) + ']');
            }

            auto out = sums_[Size - 1].tanh();

            return out;
        }

    private:
        std::array<Value, Size> weights_;
        std::array<Value, Size> products_;
        std::array<Value, Size> sums_;
        Value bias_;
};

namespace dot
{

void writeNodes(const Value &node, std::ofstream &of)
{
    of << std::setw(8) << ' ' << node.id() << " [label=\"" << node.getLabel() << "|data:" << std::dec << std::fixed
       << std::setprecision(4) << node.getData() << "|grad:" << node.getGrad() << "\", shape=\"record\"];\n";
    if (node.hasOp())
    {
        of << std::setw(8) << ' ' << node.id() << "_Op" << " [label=\"" << node.getOp() << "\"];\n";
    }

    for (const auto *child : node.getChildren())
    {
        if (!child)
        {
            continue;
        }
        writeNodes(*child, of);
    }
}

void writeEdges(const Value &node, std::ofstream &of)
{
    for (const auto *child : node.getChildren())
    {
        if (!child)
        {
            continue;
        }

        of << std::setw(8) << ' ' << child->id() << " -> ";

        of << node.id();

        if (node.hasOp())
        {
            of << "_Op";
        }

        of << '\n';
    }

    if (node.hasOp())
    {
        of << std::setw(8) << ' ' << node.id() << "_Op" << " -> " << node.id() << '\n';
    }

    if (!node.getChildren().empty())
    {
        of << '\n';
    }

    for (const auto *child : node.getChildren())
    {
        if (!child)
        {
            continue;
        }
        writeEdges(*child, of);
    }
}

void draw(const Value &root, const char *filename)
{
    std::ofstream of{filename};

    of << "digraph G {\n";
    of << "        rankdir=LR;\n";

    writeNodes(root, of);

    of << '\n';

    writeEdges(root, of);

    of << "}\n";
}

} // namespace dot

int main(int argc, char *argv[])
{
    // inputs
    std::vector<Value> x{Value{2.0, "x0"}}; //, Value{0.0, "x2"}};
    //auto x1 = Value{2.0, "x1"};
    //auto x2 = Value{0.0, "x2"};

    Neuron<1> n{5.0};

    auto o = n(x); o.setLabel("o");

    backprop(o);

    dot::draw(o, "neuron_graph.dot");
    /*
    // weights
    auto w1 = Value{-3.0, "w1"};
    auto w2 = Value{1.0, "w2"};

    // bias of neuron
    auto b = Value{6.8813735870195432, "b"};

    auto x1w1 = x1 * w1;
    auto x2w2 = x2 * w2;

    auto x1w1_plus_x2w2 = x1w1 + x2w2;

    auto n = x1w1_plus_x2w2 + b;
    n.setLabel("n");

    auto o = n.tanh();
    o.setLabel("o");

    backprop(o);

    dot::draw(o, "o_graph.dot");
    */

    // float h = 0.0001;

    // float L1, L2;

    /*
    {
        Value a{float{2.0}, "a"};
        Value b{float{-3.0}, "b"};
        Value c{float{10.0}, "c"};
        auto e = a * b; e.setLabel("e");
        auto d = e + c; d.setLabel("d");
        Value f{-2, "f"};
        auto L = d * f; L.setLabel("L");
        auto T = L.tanh(); T.setLabel("T");
        T.setData(0.7071);
        backprop(T);
        dot::draw(T, "T_graph.dot");
        L1 = L.getData();
    }
    */

    /*
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
    */

    /*
    std::cout << "L1: " << L1 << '\n';
    std::cout << "L2: " << L2 << '\n';
    std::cout << "L1-L2: " << L1 - L2 << '\n';
    std::cout << "(L1-L2)/h: " << (L1 - L2) / h << '\n';
    */

    // dot::draw(L, "L_graph.dot");

    return 0;
}
