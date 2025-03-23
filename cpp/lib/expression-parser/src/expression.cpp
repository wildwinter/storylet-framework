#include "expression_parser/expression.h"
#include "expression_parser/writer.h"
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace ExpressionParser {

// ---------------------
// Utils implementations
// ---------------------
namespace Utils {

bool MakeBool(const std::any &val) {
    if (val.type() == typeid(bool))
        return std::any_cast<bool>(val);
    if (val.type() == typeid(int))
        return std::any_cast<int>(val) != 0;
    if (val.type() == typeid(double))
        return std::any_cast<double>(val) != 0;
    if (val.type() == typeid(std::string)) {
        std::string s = std::any_cast<std::string>(val);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return (s == "true" || s == "1");
    }
    throw std::runtime_error("Type mismatch: Expecting bool");
}

double MakeNumeric(const std::any &val) {
    if (val.type() == typeid(bool))
        return std::any_cast<bool>(val) ? 1.0 : 0.0;
    if (val.type() == typeid(int))
        return static_cast<double>(std::any_cast<int>(val));
    if (val.type() == typeid(double))
        return std::any_cast<double>(val);
    if (val.type() == typeid(std::string)) {
        const std::string &s = std::any_cast<std::string>(val);
        size_t pos = 0;
        double result = 0.0;
        try {
            result = std::stod(s, &pos);
        } catch (...) {
            throw std::runtime_error("Type mismatch: Expecting number but got '" + s + "'");
        }
        if (pos != s.size())
            throw std::runtime_error("Type mismatch: Expecting number but got '" + s +"'");
        return result;
    }
    throw std::runtime_error("Type mismatch: Expecting number");
}

std::string MakeString(const std::any &val) {
    if (val.type() == typeid(std::string))
        return std::any_cast<std::string>(val);
    if (val.type() == typeid(bool))
        return std::any_cast<bool>(val) ? "true" : "false";
    if (val.type() == typeid(int))
        return std::to_string(std::any_cast<int>(val));
    if (val.type() == typeid(double))
        return std::to_string(std::any_cast<double>(val));
    throw std::runtime_error("Type mismatch: Expecting string");
}

std::any MakeTypeMatch(const std::any &leftVal, const std::any &rightVal) {
    if (leftVal.type() == typeid(bool))
        return MakeBool(rightVal);
    if (leftVal.type() == typeid(int) || leftVal.type() == typeid(double))
        return MakeNumeric(rightVal);
    if (leftVal.type() == typeid(std::string))
        return MakeString(rightVal);
    throw std::runtime_error("Type mismatch: unrecognised type");
}

bool AnyEquals(const std::any &a, const std::any &b) {
    // First, if the types don't match, we consider them unequal.
    if (a.type() != b.type())
        return false;

    if (a.type() == typeid(int))
        return std::any_cast<int>(a) == std::any_cast<int>(b);
    else if (a.type() == typeid(double))
        return std::any_cast<double>(a) == std::any_cast<double>(b);
    else if (a.type() == typeid(bool))
        return std::any_cast<bool>(a) == std::any_cast<bool>(b);
    else if (a.type() == typeid(std::string))
        return std::any_cast<std::string>(a) == std::any_cast<std::string>(b);
    else
        throw std::runtime_error("Unsupported type for equality comparison");
}

std::string FormatBoolean(bool val) {
    return val ? "true" : "false";
}

std::string FormatNumeric(double num) {
    if (std::fmod(num, 1.0) == 0.0)
        return std::to_string(static_cast<int>(num));
    std::ostringstream oss;
    oss << num;
    return oss.str();
}

std::string FormatString(const std::string &val) {
    switch (Writer::getStringFormat())
    {
        case STRING_FORMAT::SINGLEQUOTE:
            return "'"+val+"'";
        case STRING_FORMAT::ESCAPED_SINGLEQUOTE:
            return "\\'"+val+"\\'";
        case STRING_FORMAT::ESCAPED_DOUBLEQUOTE:
            return "\\\""+val+"\\\"";
        case STRING_FORMAT::DOUBLEQUOTE:
        default:
            return "\""+val+"\"";
    }
}

std::string FormatValue(const std::any &val) {
    if (val.type() == typeid(bool))
        return FormatBoolean(std::any_cast<bool>(val));
    if (val.type() == typeid(int))
        return std::to_string(std::any_cast<int>(val));
    if (val.type() == typeid(double))
        return FormatNumeric(std::any_cast<double>(val));
    if (val.type() == typeid(std::string))
        return FormatString(std::any_cast<std::string>(val));
    return "";
}

} // namespace Utils

// ---------------------
// BinaryOp implementations
// ---------------------
BinaryOp::BinaryOp(const std::string &name, std::shared_ptr<ExpressionNode> left, const std::string &op,
                   std::shared_ptr<ExpressionNode> right, int precedence)
    : ExpressionNode(name, precedence), Left(left), Right(right), Op(op) {}

std::any BinaryOp::Evaluate(const Context &context, std::vector<std::string>* dumpEval) const {
    std::any leftVal = Left->Evaluate(context, dumpEval);

    auto [shortCircuit, shortCircuitResult] = ShortCircuit(leftVal);
    if (shortCircuit)
    {
        if (dumpEval != nullptr)
        {
            dumpEval->push_back("Evaluated: " + Utils::FormatValue(leftVal) + " " + Op + " (ignore) = " + Utils::FormatValue(shortCircuitResult));
        }
        return shortCircuitResult;
    }

    std::any rightVal = Right->Evaluate(context, dumpEval);
    std::any result = DoEval(leftVal, rightVal);
    if (dumpEval) {
        dumpEval->push_back("Evaluated: " + Utils::FormatValue(leftVal) + " " + Op + " " +
                              Utils::FormatValue(rightVal) + " = " + Utils::FormatValue(result));
    }
    return result;
}

std::pair<bool, std::any> BinaryOp::ShortCircuit(const std::any& leftVal) const {
    return { false, std::any() };
}

std::string BinaryOp::DumpStructure(int indent) const {
    std::string indentStr(indent * 2, ' ');
    return indentStr + Name + "\n" +
           Left->DumpStructure(indent + 1) +
           Right->DumpStructure(indent + 1);
}

std::string BinaryOp::Write() const {
    std::string leftStr = Left->Write();
    std::string rightStr = Right->Write();
    if (Left->Precedence < this->Precedence)
        leftStr = "(" + leftStr + ")";
    if (Right->Precedence < this->Precedence)
        rightStr = "(" + rightStr + ")";
    return leftStr + " " + Op + " " + rightStr;
}

// Concrete BinaryOp classes
OpOr::OpOr(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("Or", left, "or", right, 40) {}

std::pair<bool, std::any> OpOr::ShortCircuit(const std::any& leftVal) const {

    bool result = Utils::MakeBool(leftVal);
    if (result)
        return {true, true};
    return { false, std::any() };
}

std::any OpOr::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeBool(leftVal) || Utils::MakeBool(rightVal);
}

OpAnd::OpAnd(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("And", left, "and", right, 50) {}

std::pair<bool, std::any> OpAnd::ShortCircuit(const std::any& leftVal) const {

    bool result = Utils::MakeBool(leftVal);
    if (!result)
        return {true, false};
    return { false, std::any() };
}

std::any OpAnd::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeBool(leftVal) && Utils::MakeBool(rightVal);
}

OpEquals::OpEquals(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("Equals", left, "==", right, 60) {}

std::any OpEquals::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    std::any rVal = Utils::MakeTypeMatch(leftVal, rightVal);
    return Utils::AnyEquals(leftVal, rVal);
}

OpNotEquals::OpNotEquals(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("NotEquals", left, "!=", right, 60) {}

std::any OpNotEquals::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    std::any rVal = Utils::MakeTypeMatch(leftVal, rightVal);
    return !Utils::AnyEquals(leftVal, rVal);
}

OpPlus::OpPlus(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("Plus", left, "+", right, 70) {}

std::any OpPlus::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeNumeric(leftVal) + Utils::MakeNumeric(rightVal);
}

OpMinus::OpMinus(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("Minus", left, "-", right, 70) {}

std::any OpMinus::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeNumeric(leftVal) - Utils::MakeNumeric(rightVal);
}

OpDivide::OpDivide(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("Divide", left, "/", right, 85) {}

std::any OpDivide::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    double numRight = Utils::MakeNumeric(rightVal);
    if (numRight == 0)
        throw std::runtime_error("Division by zero.");
    return Utils::MakeNumeric(leftVal) / numRight;
}

OpMultiply::OpMultiply(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("Multiply", left, "*", right, 80) {}

std::pair<bool, std::any> OpMultiply::ShortCircuit(const std::any& leftVal) const {

    double result = Utils::MakeNumeric(leftVal);
    if (result==0.0)
        return {true, 0.0};
    return { false, std::any() };
}

std::any OpMultiply::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeNumeric(leftVal) * Utils::MakeNumeric(rightVal);
}

OpGreaterThan::OpGreaterThan(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("GreaterThan", left, ">", right, 60) {}

std::any OpGreaterThan::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeNumeric(leftVal) > Utils::MakeNumeric(rightVal);
}

OpLessThan::OpLessThan(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("LessThan", left, "<", right, 60) {}

std::any OpLessThan::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeNumeric(leftVal) < Utils::MakeNumeric(rightVal);
}

OpGreaterThanEquals::OpGreaterThanEquals(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("GreaterThanEquals", left, ">=", right, 60) {}

std::any OpGreaterThanEquals::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeNumeric(leftVal) >= Utils::MakeNumeric(rightVal);
}

OpLessThanEquals::OpLessThanEquals(std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : BinaryOp("LessThanEquals", left, "<=", right, 60) {}

std::any OpLessThanEquals::DoEval(const std::any &leftVal, const std::any &rightVal) const {
    return Utils::MakeNumeric(leftVal) <= Utils::MakeNumeric(rightVal);
}

// ---------------------
// UnaryOp implementations
// ---------------------
UnaryOp::UnaryOp(const std::string &name, const std::string &op, std::shared_ptr<ExpressionNode> operand, int precedence)
    : ExpressionNode(name, precedence), Operand(operand), Op(op) {}

std::any UnaryOp::Evaluate(const Context &context, std::vector<std::string>* dumpEval) const {
    std::any val = Operand->Evaluate(context, dumpEval);
    std::any result = DoEval(val);
    if (dumpEval) {
        dumpEval->push_back("Evaluated: " + Op + " " + Utils::FormatValue(val) +
                              " = " + Utils::FormatValue(result));
    }
    return result;
}

std::string UnaryOp::DumpStructure(int indent) const {
    std::string indentStr(indent * 2, ' ');
    return indentStr + Name + "\n" + Operand->DumpStructure(indent + 1);
}

std::string UnaryOp::Write() const {
    std::string operandStr = Operand->Write();
    if (Operand->Precedence < this->Precedence)
        operandStr = "(" + operandStr + ")";
    return Op + " " + operandStr;
}

OpNegative::OpNegative(std::shared_ptr<ExpressionNode> operand)
    : UnaryOp("Negative", "-", operand, 90) {}

std::any OpNegative::DoEval(const std::any &val) const {
    return -Utils::MakeNumeric(val);
}

OpNot::OpNot(std::shared_ptr<ExpressionNode> operand)
    : UnaryOp("Not", "not", operand, 90) {}

std::any OpNot::DoEval(const std::any &val) const {
    return !Utils::MakeBool(val);
}

// ---------------------
// Literal node implementations
// ---------------------
LiteralBoolean::LiteralBoolean(bool val)
    : ExpressionNode("Boolean", 100), value(val) {}

std::any LiteralBoolean::Evaluate(const Context &context, std::vector<std::string>* dumpEval) const {
    if (dumpEval)
        dumpEval->push_back("Boolean: " + Utils::FormatBoolean(value));
    return value;
}

std::string LiteralBoolean::DumpStructure(int indent) const {
    std::string indentStr(indent * 2, ' ');
    return indentStr + "Boolean(" + Utils::FormatBoolean(value) + ")\n";
}

std::string LiteralBoolean::Write() const {
    return Utils::FormatBoolean(value);
}

LiteralNumber::LiteralNumber(const std::string &val)
    : ExpressionNode("Number", 100) {
    value = std::stod(val);
}

std::any LiteralNumber::Evaluate(const Context &context, std::vector<std::string>* dumpEval) const {
    if (dumpEval)
        dumpEval->push_back("Number: " + Utils::FormatNumeric(value));
    return value;
}

std::string LiteralNumber::DumpStructure(int indent) const {
    std::string indentStr(indent * 2, ' ');
    return indentStr + "Number(" + Utils::FormatNumeric(value) + ")\n";
}

std::string LiteralNumber::Write() const {
    return Utils::FormatNumeric(value);
}

LiteralString::LiteralString(const std::string &val)
    : ExpressionNode("String", 100), value(val) {}

std::any LiteralString::Evaluate(const Context &context, std::vector<std::string>* dumpEval) const {
    if (dumpEval)
        dumpEval->push_back("String: " + Utils::FormatString(value));
    return value;
}

std::string LiteralString::DumpStructure(int indent) const {
    std::string indentStr(indent * 2, ' ');
    return indentStr + "String(" + Utils::FormatString(value) + ")\n";
}

std::string LiteralString::Write() const {
    return Utils::FormatString(value);
}

Variable::Variable(const std::string &name)
    : ExpressionNode("Variable", 100), name(name) {}

std::any Variable::Evaluate(const Context &context, std::vector<std::string>* dumpEval) const {
    auto it = context.find(name);
    if (it == context.end())
        throw std::runtime_error("Variable '" + name + "' not found in context.");
    std::any value = it->second;
    if (!(value.type() == typeid(int) || value.type() == typeid(double) ||
          value.type() == typeid(bool) || value.type() == typeid(std::string)))
        throw std::runtime_error("Variable '" + name + "' must return bool, string, or numeric.");
    if (dumpEval)
        dumpEval->push_back("Fetching variable: " + name + " -> " + Utils::FormatValue(value));
    return value;
}

std::string Variable::DumpStructure(int indent) const {
    std::string indentStr(indent * 2, ' ');
    return indentStr + "Variable(" + name + ")\n";
}

std::string Variable::Write() const {
    return name;
}

// ---------------------
// FunctionCall implementation
// ---------------------
FunctionCall::FunctionCall(const std::string &funcName, const std::vector<std::shared_ptr<ExpressionNode>> &args)
    : ExpressionNode("FunctionCall", 100), funcName(funcName), args(args) {}

std::any FunctionCall::Evaluate(const Context &context, std::vector<std::string>* dumpEval) const {
    auto it = context.find(funcName);
    if (it == context.end())
        throw std::runtime_error("Function '" + funcName + "' not found in context.");
    std::any funcObj = it->second;
    if (funcObj.type() != typeid(FunctionWrapper))
        throw std::runtime_error("Context entry for '" + funcName + "' is not a function.");
    FunctionWrapper wrapper = std::any_cast<FunctionWrapper>(funcObj);
    
    std::vector<std::any> argValues;
    for (const auto &arg : args) {
        argValues.push_back(arg->Evaluate(context, dumpEval));
    }
    
    if (argValues.size() != static_cast<size_t>(wrapper.arity)) {
        std::string formattedArgs;
        for (const auto &val : argValues)
            formattedArgs += Utils::FormatValue(val) + ", ";
        if (!formattedArgs.empty())
            formattedArgs = formattedArgs.substr(0, formattedArgs.size() - 2);
        throw std::runtime_error("Function '" + funcName + "' does not support the provided arguments (" + formattedArgs + ").");
    }
    
    std::any result = wrapper.func(argValues);
    if (!(result.type() == typeid(int) || result.type() == typeid(double) ||
          result.type() == typeid(bool) || result.type() == typeid(std::string)))
        throw std::runtime_error("Function '" + funcName + "' must return bool, string, or numeric.");
    
    if (dumpEval) {
        std::string formattedArgs;
        for (const auto &val : argValues)
            formattedArgs += Utils::FormatValue(val) + ", ";
        if (!formattedArgs.empty())
            formattedArgs = formattedArgs.substr(0, formattedArgs.size() - 2);
        dumpEval->push_back("Called function: " + funcName + "(" + formattedArgs +
                              ") = " + Utils::FormatValue(result));
    }
    
    return result;
}

std::string FunctionCall::DumpStructure(int indent) const {
    std::string indentStr(indent * 2, ' ');
    std::string outStr = indentStr + "FunctionCall(" + funcName + ")\n";
    for (const auto &arg : args)
        outStr += arg->DumpStructure(indent + 1);
    return outStr;
}

std::string FunctionCall::Write() const {
    std::string argsStr;
    for (size_t i = 0; i < args.size(); ++i) {
        argsStr += args[i]->Write();
        if (i < args.size() - 1)
            argsStr += ", ";
    }
    return funcName + "(" + argsStr + ")";
}

} // namespace ExpressionParser