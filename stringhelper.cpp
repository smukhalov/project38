#include "stringhelper.h"

const std::string& BLANK_CHARS = "\t\n\v\f\r ";

std::string_view Trim(std::string_view part){
	size_t first_pos_not_blank = part.find_first_not_of(BLANK_CHARS);
	size_t last_pos_not_blank = part.find_last_not_of(BLANK_CHARS);

	return part.substr(first_pos_not_blank, last_pos_not_blank - first_pos_not_blank + 1);
}
