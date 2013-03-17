/*
 * stl_import.cpp
 *
 *  Created on: Mar 11, 2013
 *      Author: cds
 */

#include <algorithm>
#include <exception>
#include <cctype>
#include <sstream>
#include <valarray>

#include "stl_import.h"
#include "stl_import_exception.h"

using std::istream;
using std::vector;
using std::exception;
using std::stringstream;
using maths::vector3d;
using maths::triangle3d;

void stl_import::_read(istream& stream)
{
	ms_line_num = 1;

	static size_t bufsize = 256;
	char buf[256];
	try
	{
		while (stream.good())
		{
			stream.getline(buf, bufsize);
			_read_element(buf);

			ms_line_num++;
		}
	}
	catch (exception& ex)
	{
		std::cout << "Crap, got an error: " << ex.what();
		throw ex;
	}
}

//static
double stl_import::_read_str_double(const std::string& dbl_str)
{
	std::istringstream i(dbl_str);

	double d;
	if (!(i >> d))
		throw stl_import_exception("stl_import::_read_str_double " + dbl_str);

	return d;
}

//static
vector3d stl_import::_read_vector(istream& is)
{
	// Tokenize string as array of non-whitespace strs
	std::vector<std::string> n_strs;
	std::string n_str;
	while (std::getline(is, n_str, ' '))
		if (!n_str.empty())
			n_strs.push_back(n_str);

	if (n_strs.size() != 3)
	{
		std::ostringstream err_ss;
		err_ss << "Unexpected number of vertices (got"
			   << n_strs.size() << " expected 3)";

		throw stl_import_exception(err_ss.str());
	}

	std::valarray<double> facet_pts(0.0, 3);
	for (size_t i = 0 ; i < 3 ; i++)
		facet_pts[i] = _read_str_double(n_strs[i]);

	return vector3d(facet_pts);
}

void stl_import::_read_solid(istream& is)
{
	// Assumes that we've already read the 'solid' element.
	// Just use the first non-whitespace string we encounter
	// as the
	std::string name;
	while (std::getline(is, name, ' '))
	{
		if (!name.empty())
		{
			const_cast<std::string&>(m_solid_name) = name;
			break;
		}
	}
}

void stl_import::_read_facet(istream& is)
{
	if (!ms_last_facet_normal.is_null())
	{
		std::ostringstream os;
		os << "Already have facet normal " << ms_last_facet_normal << " at line " << ms_line_num;

		throw stl_import_exception(os.str());
	}

	// Assume that we've already read "facet"
	std::string normal_str;
	if (!std::getline(is, normal_str, ' ') || normal_str.compare("normal") != 0)
	{
		std::ostringstream exp_ss;
		exp_ss << "Error reading facet (expected \"normal\" got: " << normal_str
				<< " on line " << ms_line_num;

		throw stl_import_exception(exp_ss.str());
	}

	ms_last_facet_normal = _read_vector(is);
}

void stl_import::_read_vertex(istream& is)
{
	if (ms_cur_triangle.size() > 2)
	{
		std::ostringstream ex_sstr;
		ex_sstr << "Got extra vertex at line " << ms_line_num;

		throw stl_import_exception(ex_sstr.str());
	}

	const vector3d v = _read_vector(is);
	ms_cur_triangle.push_back(v);
}

void stl_import::_read_endfacet(istream& is)
{
	if (ms_last_facet_normal.is_null())
	{
		std::ostringstream exp_os;
		exp_os << "Didn't get facet normal reading endfacet at line " << ms_line_num;

		throw stl_import_exception(exp_os.str());
	}

	if (ms_cur_triangle.size() != 3)
	{
		std::ostringstream exp_os;
		exp_os << "Incorrect number of facet vertices (expected 3 got " << ms_cur_triangle.size()
				<< " reading endfacet at line " << ms_line_num;

		throw stl_import_exception(exp_os.str());
	}

	const vector3d& a = ms_cur_triangle[0];
	const vector3d& b = ms_cur_triangle[1];
	const vector3d& c = ms_cur_triangle[2];
	const triangle3d facet(a, b, c);

	// Make sure the normals are close
	if (!maths::close(facet.normal().distance_sq(ms_last_facet_normal), 0.0, 1.0e-8))
		std::cout << "WARNING: facet normals are different at line " << ms_line_num << std::endl
			<< "Expected: " << ms_last_facet_normal << " got " << facet.normal() << std::endl;

	// Add the facet to our bag of facets
	m_facets.push_back(facet);

	// Reset the state
	ms_got_outer_loop = false;
	ms_num_vertices = false;
	ms_last_facet_normal = vector3d();
	ms_cur_triangle.clear();
}

void stl_import::_read_outer_loop(istream& is)
{
	// We've already read "outer"
	if (ms_got_outer_loop)
	{
		std::ostringstream exp_ss;
		exp_ss << "Duplicate outer loop at line " << ms_line_num;

		throw stl_import_exception(exp_ss.str());
	}

	std::string tok;
	while (std::getline(is, tok, ' '))
	{
		if (tok.empty())
			continue;

		if (tok.compare("loop") == 0)
		{
			ms_got_outer_loop = true;
		}
		else
		{
			std::ostringstream exp_ss;
			exp_ss << "Unknown token " << tok
					<< " encountered at line " << ms_line_num;

			throw stl_import_exception(exp_ss.str());
		}
	}
	if (!ms_got_outer_loop)
	{
		std::ostringstream exp_ss;
		exp_ss << "No \"loop\" token for \"outer\" token at line " << ms_line_num;

		throw stl_import_exception(exp_ss.str());
	}
}

void stl_import::_read_end_loop(istream& is)
{
	if (!ms_got_outer_loop)
	{
		std::ostringstream exp_ss;
		exp_ss << "Unexpected endloop token encountered at line " << ms_line_num;

		throw stl_import_exception(exp_ss.str());
	}

	ms_got_outer_loop = false;
}

void stl_import::_read_endsolid(istream& is)
{
	// For now, do nothing...
}

void stl_import::_read_element(const std::string& el)
{
	if (el.empty())
		return;

	// Make element lowercase
	std::string el_lower = el;
	std::transform(el_lower.begin(), el_lower.end(), el_lower.begin(), ::tolower);
	std::replace(el_lower.begin(), el_lower.end(), '\t', ' ');	// tabs to spaces

	// Tokenize string
	std::istringstream ss(el_lower);
	std::string tok;

	while (std::getline(ss, tok, ' '))
	{
		if (tok.empty())
			continue;

		if (tok.compare("solid") == 0)
			_read_solid(ss);
		else if (tok.compare("outer") == 0)
			_read_outer_loop(ss);
		else if (tok.compare("facet") == 0)
			_read_facet(ss);
		else if (tok.compare("endfacet") == 0)
			_read_endfacet(ss);
		else if (tok.compare("vertex") == 0)
			_read_vertex(ss);
		else if (tok.compare("outer") == 0)
			_read_outer_loop(ss);
		else if (tok.compare("endloop") == 0)
			_read_end_loop(ss);
		else if (tok.compare("endsolid") == 0)
			_read_endsolid(ss);
		else
		{
			std::ostringstream exp_ss;
			exp_ss << "Unknown token: " << tok << " encountered on line " << ms_line_num;

			throw stl_import_exception(exp_ss.str());
		}
	}
}

stl_import::stl_import(istream& stream)
: ms_line_num(1)
, ms_got_outer_loop(false)
, ms_num_vertices(0)
{
	_read(stream);
}
