#include <numeric>
#include <sstream>
#include <fstream>
#include <iterator>

#include <getline_crlf_lf.h>
#include <finally.h>
#include <make_unique.h>

#include "stl_importer.h"

using namespace std;
using namespace stl_util;	// change this...
using namespace stlutil;
using namespace maths;


//////////////////////////
// ascii_stl_reader

ascii_stl_reader::ascii_stl_reader(istream& istream)
: m_istream(istream)
, m_done(false)
{

}

string ascii_stl_reader::get_next_line()
{
	string line;

	while (line.empty())
	{
		getline_crlf_lf(m_istream, line);

		if (m_istream.eof())
			break;
	}

	prep_line_(line);

	return line;
}

//static
void ascii_stl_reader::prep_line_(string& line)
{
	std::transform(line.begin(), line.end(), line.begin(), ::tolower);	// Make element lowercase
	std::replace(line.begin(), line.end(), '\t', ' ');					// tabs to spaces

	// trim spaces from line
	auto left_spaces_end = std::find_if(line.begin(), line.end(), [](char c) { return !isspace(c); });
	line.erase(line.begin(), left_spaces_end);

	auto right_spaces_end = std::find_if(line.rbegin(), line.rend(), [](char c) { return !isspace(c); });
	line.erase(right_spaces_end.base(), line.end());
}

//static
vector<string> ascii_stl_reader::tokenize_line_(const string& line)
{
	vector<string> tokens;

	stringstream ss_tok(line);
	string tok;

	while (std::getline(ss_tok, tok, ' '))
	{
		if (!tok.empty())	// shouldn't happen if line is trimmed
			tokens.push_back(tok);
	}

	return tokens;
}

bool ascii_stl_reader::read_header(string& name)
{
	std::string solid_line;
	getline_crlf_lf(m_istream, solid_line);

	// It doesn't make sense to do most of the things that we do in prep_line
	// for "solid", but we still want to convert tabs to spaces for tokenization
	std::replace(solid_line.begin(), solid_line.end(), '\t', ' ');

	auto tokens = tokenize_line_(solid_line);

	if (tokens.empty() || tokens[0] != "solid")
		return false;

	// Concatenate the rest of the tokens (if there are any) into the solid name
	string solid_name = accumulate(tokens.begin() + 1, tokens.end(), string(),
		[](string out_name, const string& tok)
		{
			if (!out_name.empty())
				out_name += " ";

			out_name += tok;

			return out_name;
		});

	name = solid_name;

	return true;
}

bool ascii_stl_reader::read_facet(triangle3d& triangle, vector3d& normal)
{
	string line = get_next_line();

	if (line == "endsolid" || line.empty())
	{
		m_done = true;
		return false;
	}

	{
		// Read "facet normal"
		vector<string> fn_tokens = tokenize_line_(line);
		if (fn_tokens.size() != 5)
			return false;

		if (fn_tokens[0] != "facet")
			return false;
		if (fn_tokens[1] != "normal")
			return false;

		auto fn_begin = fn_tokens.begin() + 2;

		for (size_t i = 0 ; i < 3 ; i++)
		{
			istringstream double_str(*(fn_begin + i));
			if (!(double_str >> normal[i]))
				return false;
		}
	}

	line = get_next_line();
	if (done())
		return false;

	{
		// Read "outer loop"
		if (line != "outer loop")
			return false;
	}

	// Read vertices
	vector<maths::vector3d> t_verts(3);

	for (int i = 0 ; i < 3 ; i++)
	{
		line = get_next_line();
		auto tokens = tokenize_line_(line);

		if (tokens[0] != "vertex")
			return false;

		auto v_tok = tokens.begin() + 1;

		for (int j = 0 ; j < 3 ; j++)
		{
			istringstream ss(*(v_tok + j));
			if (!(ss >> t_verts[i][j]))
				return false;
		}
	}

	triangle = triangle3d(t_verts[0], t_verts[1], t_verts[2]);

	line = get_next_line();
	if (done())
		return false;

	// Read "endloop" and "endfacet"
	if (line != "endloop")
		return false;

	line = get_next_line();
	if (done())
		return false;

	if (line != "endfacet")
		return false;

	return true;
}

bool ascii_stl_reader::done() const
{
	return m_done || m_istream.eof();
}

size_t ascii_stl_reader::get_file_facet_count()
{
	size_t facet_count = 0;

	m_istream.seekg(0);	// rewind the stream

	while (!m_istream.eof())
	{
		string line = get_next_line();
		if (line.size() < 5)
			continue;

		string facet_substr(line.begin(), line.begin() + 5);
		if (facet_substr == "facet")
			facet_count++;
	}

	m_istream.seekg(0);

	return facet_count;
}

//////////////////////////////
// binary_stl_reader
binary_stl_reader::binary_stl_reader(istream& istream)
: m_istream(istream)
, m_num_facets(0)
{
	// Unfortunately, there is no way to test if the stream was opened in binary mode...
}

bool binary_stl_reader::read_header(string& name)
{
	char header_buf[80];
	m_istream.read(header_buf, 80);

	if (!m_istream.good())
		return false;

	name = header_buf;

	// Sometimes there is junk after the name.  Chop of any non-printable characters.
	auto npc = std::find_if(name.begin(), name.end(), [](char c) { return !::isprint(c); });
	if (npc != name.end())
		name.resize(std::distance(name.begin(), npc));

	// Gratuitous whitespace
	auto spaces = std::adjacent_find(name.begin(), name.end(), [](char a, char b) { return ::isspace(a) && ::isspace(b); });
	if (spaces != name.end())
		name.resize(std::distance(name.begin(), spaces));

	// Read the expected number of triangles while we're at it
	// It should be immediately after the header
	char facet_count_buf[4];
	m_istream.read(facet_count_buf, 4);

	if (!m_istream.good())
		return false;

	auto num_facets = reinterpret_cast<uint32_t*>(facet_count_buf);
	m_num_facets = *num_facets;

	return true;
}

bool binary_stl_reader::read_facet(triangle3d& triangle, vector3d& normal)
{
	char vector_buf[12];

	// read normal
	m_istream.read(vector_buf, 12);
	if (!m_istream.good())
		return false;

	auto vector_arr = reinterpret_cast<float*>(vector_buf);
	normal = maths::convert<double, float>(vector3f(vector_arr, 3));

	// Read facet vertices
	vector<vector3d> t_verts(3);

	for (int i = 0 ; i < 3 ; i++)
	{
		m_istream.read(vector_buf, 12);
		if (!m_istream.good())
			return false;

		auto tv_arr = reinterpret_cast<float*>(vector_buf);
		t_verts[i] = maths::convert<double, float>(vector3f(tv_arr, 3));
	}

	triangle = triangle3d(t_verts[0], t_verts[1], t_verts[2]);

	// Read (and throw away) attribute byte count thing
	char attrib_count_buf[2];
	m_istream.read(attrib_count_buf, 2);

	return m_istream.good();
}

bool binary_stl_reader::done() const
{
	return m_istream.eof();
}

size_t binary_stl_reader::get_file_facet_count()
{
	string nop_name;
	read_header(nop_name);	// m_num_facets are initialized in read_header()

	return m_num_facets;
}

////////////////////////
// stl_importer

stl_importer::stl_importer(const shared_ptr<istream>& istream)
: m_istream(istream)
, m_expected_facet_count(0)
, m_facets_read(0)
{
	m_stl_reader = create_stl_reader_();
	m_expected_facet_count = m_stl_reader->get_file_facet_count();
}

stl_importer::stl_importer(const string& filename)
: m_expected_facet_count(0)
, m_facets_read(0)
{
	auto stl_ifstream = make_shared<ifstream>();

	stl_ifstream->open(filename, std::fstream::binary);

	if (!stl_ifstream->is_open())
		throw std::runtime_error("Error opening file");

	m_istream = stl_ifstream;

	m_stl_reader = create_stl_reader_();
	m_expected_facet_count = m_stl_reader->get_file_facet_count();
}

unique_ptr<stl_reader_interface> stl_importer::create_stl_reader_()
{
	// Peek at the first line of the file
	m_istream->seekg(0);

	// Rewind the stream when we're done
	finally f([this]() { m_istream->seekg(0); });

	string line;
	getline_crlf_lf(*m_istream, line);
	ascii_stl_reader::prep_line_(line);

	string solid_substr(line.begin(), line.begin() + 5);
	if (solid_substr == "solid")
	{
		// Because some assholes think it's OK to start a binary STL with "solid"...
		getline_crlf_lf(*m_istream, line);

		ascii_stl_reader::prep_line_(line);
		auto tokens = ascii_stl_reader::tokenize_line_(line);

		if (tokens.size() > 2 && tokens[0] == "facet" && tokens[1] == "normal")
			return make_unique<ascii_stl_reader>(*m_istream);
	}

	return make_unique<binary_stl_reader>(*m_istream);
}
