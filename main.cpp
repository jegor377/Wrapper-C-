#include <iostream>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <exception>
#include <string>
#include <cstring>
#include <sstream>
#include <iterator>
#include <streambuf>
#include <algorithm>

typedef std::string String;
typedef std::vector<String> StringList;
typedef StringList DirEntryList;

struct ModuleFile {
	String module_name;
	String module_content;
};
typedef std::vector<ModuleFile> ModuleFileList;

const String EMPTY_STRING = "";

void show_usage() {
	std::cout<<"Wrapper for C++ created by Igor Santarek (github: jegor377) MIT 2018."<<std::endl<<std::endl;
	std::cout<<"Usage:"<<std::endl;
	std::cout<<"wcpp <module name> <init file>"<<std::endl<<std::endl;
	std::cout<<"<module name> - name of the  directory   witch contains all library files."<<std::endl<<std::endl;
	std::cout<<"It  can   also  be   treated  as   a  path  to  the   module    directory."<<std::endl;
	std::cout<<"If so, then output module name  is  the  last  directory name in the path."<<std::endl<<std::endl;
	std::cout<<"<init file> - NAME of file that can  import  all  the files inside module."<<std::endl;
	std::cout<<"Operation results by creating output <module name> header file (hpp) witch"<<std::endl;
	std::cout<<"contains    all   files'    insides      from   the    module   directory."<<std::endl;
}

DirEntryList read_dir(char* dir_path) {
	DirEntryList result;
	DIR* dir;
	struct dirent* ent;
	if((dir = opendir(dir_path)) != NULL) {
		while((ent = readdir(dir)) != NULL) {
			if(String(ent->d_name) != "." && String(ent->d_name) != "..") {
				result.push_back(ent->d_name);
			}
		}
		closedir(dir);
	} else throw "Directory read error.";
	return result;
}

void replaceAll(String& str, const String& from, const String& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

StringList split(const String &s, char delim) {
    StringList elems;
    std::stringstream ss(s);
    String item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

// results in last path element.
String get_element_name(String element_path) {
	const String WINDOWS_DIR_CHAR = "\\";
	const String DIR_DELIMITER_CHAR = "/";
	// Make it a bit easier
	String dir_name = element_path;
	replaceAll(dir_name, WINDOWS_DIR_CHAR, DIR_DELIMITER_CHAR);
	// Try to split path by path delimiter character.
	dir_name = split(element_path, DIR_DELIMITER_CHAR[0]).back();
	return dir_name;
}

String get_file_extension(String file_path) {
	auto element_name = get_element_name(file_path);
	auto element_extension = split(element_name, '.');
	return element_extension.back();
}

String get_file_name(String file_path) {
	auto element_name = get_element_name(file_path);
	auto element_extension = split(element_name, '.');
	return element_extension.front();
}

void save_result_file(String module_name, String result_file_content) {
	const String OUTPUT_FILE_EXTENSION = ".hpp";
	String filename = get_element_name(module_name) + OUTPUT_FILE_EXTENSION;
	std::cout<<"Output file path: "<<filename<<std::endl;
	std::cout<<filename<<std::endl;
	std::fstream result_file(filename.c_str(), std::ios::out);
	if(result_file.good()) {
		std::cout<<"Creating output file..."<<std::endl;
		result_file<<result_file_content;
		result_file.close();
	} else throw "An error occurred. Cannot to save output file.";
}

bool is_module_file(String module_file_name) {
	return get_file_extension(module_file_name) == "hpp";
}

String load_entire_file(std::ifstream& file) {
	String content;

	file.seekg(0, std::ios::end);   
	content.reserve(file.tellg());
	file.seekg(0, std::ios::beg);

	content.assign(std::istreambuf_iterator<char>(file),
				   std::istreambuf_iterator<char>());
	return content;
}

ModuleFileList load_module_files(const String& dir_path, DirEntryList dir_entries) {
	ModuleFileList result;
	for(auto file_path : dir_entries) {
		if(is_module_file(file_path)) {
			String dir_file_path = dir_path + "/" + file_path;
			std::ifstream module_file(dir_file_path.c_str());
			if(module_file.good()) {
				auto content = load_entire_file(module_file);

				ModuleFile file = {get_file_name(file_path), content};

				result.push_back(file);
				module_file.close();
			}
		}
	}
	return result;
}

String replace_imports(const String& init_content, const ModuleFileList& module_files) {
	std::cout<<"Replacing imports..."<<std::endl;
	String result = init_content;
	if(result.empty()) return result;
	size_t start_pos = 0;
	const String IMPORT_KEYWORD = "@import ";
	const char END_NAME_CHARACTER = ';';
	while((start_pos = result.find(IMPORT_KEYWORD, start_pos)) != std::string::npos) {
		result.replace(start_pos, IMPORT_KEYWORD.length(), EMPTY_STRING);

		String module_name = EMPTY_STRING;
		size_t name_end_pos = start_pos;
		while(result[name_end_pos] != END_NAME_CHARACTER) {module_name += result[name_end_pos]; name_end_pos++;}
		
		auto module_iter = std::find_if(module_files.begin(), module_files.end(), [module_name](ModuleFile const& mf){
			return mf.module_name == module_name;
		});
		if(module_iter != module_files.end()) {
			int module_index = std::distance(module_files.begin(), module_iter);
			auto module_file = module_files[module_index];
			result.replace(start_pos, module_name.length()+1, module_file.module_content);
			start_pos += module_file.module_content.length()+1; // +1 because END_NAME_CHARACTER is on the end of the imported module name ;)
		}
	}
	return result;
}

void process_init_file(const String& module_name, const String& init_name, const ModuleFileList& module_files) {
	std::cout<<"Searching for init file..."<<std::endl;
	auto module_init_iter = std::find_if(module_files.begin(), module_files.end(), [init_name](ModuleFile const& mf){
		return mf.module_name == init_name;
	});
	if(module_init_iter != module_files.end()) {
		int init_index = std::distance(module_files.begin(), module_init_iter);
		auto init_module_file = module_files[init_index];
		auto result_content = replace_imports(init_module_file.module_content, module_files);
		if(result_content.length() > 0) {
			save_result_file(module_name, result_content);
		} else throw "Result content is empty.";
	} else throw "Module file not found.";
}

int process_module(char* module_dir, char* init_name) {
	try {
		std::cout<<"Loading module files..."<<std::endl;
		auto module_files = load_module_files(module_dir, read_dir(module_dir));
		process_init_file(get_element_name(module_dir), init_name, module_files);
	} catch(const char* err) {
		std::cerr<<err<<std::endl;
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		show_usage();
		return 0;
	}
	const int MODULE_NAME_ID = 1;
	const int MODULE_INIT_NAME_ID = 2;
	return process_module(argv[MODULE_NAME_ID], argv[MODULE_INIT_NAME_ID]);
}