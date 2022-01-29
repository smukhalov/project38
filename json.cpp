#include "json.h"

using namespace std;

namespace Json {

  Document::Document(Node root) : root(move(root)) {
  }

  const Node& Document::GetRoot() const {
    return root;
  }

  Node LoadNode(istream& input);

  Node LoadArray(istream& input) {
    vector<Node> result;

    for (char c; input >> c && c != ']'; ) {
      if (c != ',') {
        input.putback(c);
      }
      result.push_back(LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadBool(istream& input, char c){
	  std::string bool_string;
	  bool_string += c;
	  if(c == 't'){
		  for(int i = 0; i < 3; i++){
			  bool_string += input.get();
		  }

		  if(bool_string == "true"){
			  return Node(BoolValue(true));
		  }

		  throw invalid_argument("Wait true, get " + bool_string);
	  }

	  for(int i = 0; i < 4; i++){
		  bool_string += input.get();
	  }

	  if(bool_string == "false"){
		  return Node(BoolValue(false));
	  }

	  throw invalid_argument("Wait false, get " + bool_string);
  }

  Node LoadIntOrDouble(istream& input, int sign){
	  //string s_int_part;
	  //string s_fract_part;
	  bool dot_found = false;

	  int result_int = 0;
	  double result_double = 0.0;
	  int frac_mult = 10;
	  while (isdigit(input.peek()) || input.peek() == '.') {
		  char c = input.get();
		  if(c == '.'){
			  dot_found = true;
			  result_double = result_int;
			  continue;
		  }

		  if(dot_found){
			  //s_fract_part += c;
			  result_double += double(c - '0') / frac_mult;
			  frac_mult *= 10;
		  } else {
			  //s_int_part += c;
			  result_int *= 10;
			  result_int += c - '0';
		  }
	  }

	  if(!dot_found){
		  /*int result = 0;
		  for (const char c: s_int_part) {
			result *= 10;
			result += c - '0';
		  }*/
		  return Node(IntValue(sign * result_int));
		  //return Node(IntValue(stoi(s_int_part)));
	  }

	  //return Node(DoubleValue(sign * stod(s_int_part + "." + s_fract_part)));
	  return Node(DoubleValue(sign * result_double));
  }

  Node LoadString(istream& input) {
    string line;
    getline(input, line, '"');
    return Node(move(line));
  }

  Node LoadDict(istream& input) {
    map<string, Node> result;

    for (char c; input >> c && c != '}'; ) {
      if (c == ',') {
        input >> c;
      }

      string key = LoadString(input).AsString();
      input >> c;
      result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
      return LoadArray(input);
    } else if (c == '{') {
      return LoadDict(input);
    } else if (c == '"') {
      return LoadString(input);
    } else if (c == 't' || c == 'f') {
      return LoadBool(input, c);
    } else if (isdigit(c) || c == '.' || c == '+' || c == '-') {
    	int sign = 1;
    	if(isdigit(c) || c == '.'){
    		input.putback(c);
    	} else if(c == '-'){
    		sign = -1;
    	}

        return LoadIntOrDouble(input, sign);
    } else {
    	//input.putback(c);
    	//return Node(IntValue(0));
    	throw invalid_argument("unexpected char - " + c);
      //input.putback(c);
      //return LoadIntOrDouble(input);
    }
  }

  Document Load(istream& input) {
    return Document{LoadNode(input)};
  }

}
