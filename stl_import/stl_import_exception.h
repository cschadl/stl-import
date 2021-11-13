#ifndef STL_IMPORT_EXCEPTION_H
#define STL_IMPORT_EXCEPTION_H

#include <exception>
#include <string>

class stl_import_exception : public std::exception
{
private:
	const std::string m_err_str;

public:
	stl_import_exception(const std::string& what) : m_err_str(what) { }
	virtual const char* what() const throw() { return m_err_str.c_str(); }
	virtual ~stl_import_exception() throw() { }
};

#endif
