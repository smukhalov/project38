#pragma once

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace Json {
	struct IntValue {
		size_t value;
		explicit IntValue(size_t value_): value(value_) {}

		operator size_t() const{
			return value;
		}
	};

	struct DoubleValue {
		double value;
		explicit DoubleValue(double value_): value(value_) {}

		operator double() const{
			return value;
		}
	};

	struct BoolValue {
		bool value;
		explicit BoolValue(bool value_): value(value_) {}

		operator bool() const{
			return value;
		}
	};



  class Node : std::variant<std::vector<Node>,
                            std::map<std::string, Node>,
							IntValue,
							DoubleValue,
							BoolValue,
                            std::string> {
  public:
    using variant::variant;

    const auto& AsArray() const {
      return std::get<std::vector<Node>>(*this);
    }
    const auto& AsMap() const {
      return std::get<std::map<std::string, Node>>(*this);
    }
    int AsInt() const {
      return std::get<IntValue>(*this);
    }
    double AsDouble() const {
    	return std::get<DoubleValue>(*this);
	}
    bool AsBool() const {
	  return std::get<BoolValue>(*this);
	}
    const auto& AsString() const {
      return std::get<std::string>(*this);
    }

    bool IsBool() const {
	  return std::holds_alternative<BoolValue>(*this);
	}

    bool IsInt() const {
	  return std::holds_alternative<IntValue>(*this);
	}

    bool IsDouble() const {
	  return std::holds_alternative<DoubleValue>(*this);
	}

    bool IsString() const {
	  return std::holds_alternative<std::string>(*this);
	}

    bool IsMap() const {
	  return std::holds_alternative<std::map<std::string, Node>>(*this);
	}

    bool IsArray() const {
	  return std::holds_alternative<std::vector<Node>>(*this);
	}
  };

  class Document {
  public:
    explicit Document(Node root);

    const Node& GetRoot() const;

  private:
    Node root;
  };

  Document Load(std::istream& input);

}
