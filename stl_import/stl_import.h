/*
 * stl_import.h
 *
 *  Created on: Mar 2, 2013
 *      Author: cds
 */

#ifndef STL_IMPORT_H_
#define STL_IMPORT_H_

#include <vector>
#include <istream>
#include <string>

#include <geom.h>

class stl_import
{
protected:
	std::vector<maths::triangle3d>	m_facets;
	const std::string				m_solid_name;

	// state
	size_t							ms_line_num;
	bool							ms_got_outer_loop;	// did we get an 'outer loop' element?
	short							ms_num_vertices;
	maths::vector3d					ms_last_facet_normal;
	std::vector<maths::vector3d>	ms_cur_triangle;

	static const maths::vector3d	NO_FACET_NORMAL;

	/** Helper function for converting a string to a double */
	static double _read_str_double(const std::string& dbl_str);

	/** Helper function for reading in a vector from a stream */
	static maths::vector3d _read_vector(std::istream& is);

	void	_read(std::istream& stream);
	void	_read_element(const std::string& el);	// reads a single line from the STL file

	/**	Reads a 'solid' element from the stream.
	 * 	This sets m_solid_name.  The solid can be empty.
	 */
	void _read_solid(std::istream& is);

	/** Reads a 'facet' element from the stream.
	 *  @returns the normal of the facet that was read.
	 *  @throws std::exception if there was an invalid facet, or if
	 *  		the facet was encountered in an invalid state.
	 */
	void	_read_facet(std::istream& is);

	/** Reads a 'vertex' element from the stream
	 *  @return the vertex that was read.
	 *  @throws std::exception if there was an invalid vertex, or
	 *  		if the vertex was encountered in an invalid state
	 */
	void	_read_vertex(std::istream& is);

	/** Reads the 'endfacet' element from the stream */
	void 	_read_endfacet(std::istream& is);

	/** Reads an 'outer loop' element from the stream
	 *  This just throws if 'outer' isn't followed by 'loop',
	 *  or if the outer loop element was encountered in an
	 *  invalid state.
	 */
	void _read_outer_loop(std::istream& is);

	/** Reads an 'endloop' element from the stream
	 *  This just throws if the endloop element was encountered
	 *  in an invalid state.
	 */
	void _read_end_loop(std::istream& is);

	/**
	 * Reads the 'endsolid' element from the stream
	 * Doesn't really do anything other than verify that the
	 * state of the importer is correct for the endsolid element.
	 */
	void _read_endsolid(std::istream& is);

public:
	// constructors
	stl_import(std::istream& stream);

	/** Gets the name of the solid as defined in the STL file.
	 *  Note that the name will be converted to lowercase.
	 */
	std::string get_name() const { return m_solid_name; }

	template <typename OutputIterator>
	void collect_facets(OutputIterator oi);

	const std::vector<maths::triangle3d>&	get_facets() const { return m_facets; }
};

template <typename OutputIterator>
void stl_import::collect_facets(OutputIterator oi)
{
	std::vector<maths::triangle3d>::const_iterator it = m_facets.begin();
	for ( ; it != m_facets.end() ; ++it)
		*oi++ = *it;
}

#endif /* STL_IMPORT_H_ */
