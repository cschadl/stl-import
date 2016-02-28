#ifndef STL_IMPORTER_H_
#define STL_IMPORTER_H_

#include <memory>

#include "geom.h"

namespace stl_util
{

class stl_reader_interface
{
public:
	virtual bool read_header(std::string& name) = 0;
	virtual bool read_facet(maths::triangle3d& triangle, maths::vector3d& normal) = 0;
	virtual bool done() const = 0;

	virtual size_t get_file_facet_count() = 0;

	virtual ~stl_reader_interface() { }
};

class ascii_stl_reader : public stl_reader_interface
{
private:
	std::istream&	m_istream;
	bool			m_done;

	static void prep_line_(std::string& line);	// converts tabs to spaces and makes all characters lowercase
	static std::vector<std::string> tokenize_line_(const std::string& line);

	std::string get_next_line();

public:
	ascii_stl_reader(std::istream& istream);

	bool read_header(std::string& name) override;
	bool read_facet(maths::triangle3d& triangle, maths::vector3d& normal) override;
	bool done() const override;

	size_t get_file_facet_count() override;
};

class binary_stl_reader : public stl_reader_interface
{
private:
	std::istream&	m_istream;
	std::uint32_t	m_num_facets;

public:
	binary_stl_reader(std::istream& istream);	// throws if stream is not binary

	bool read_header(std::string& name) override;
	bool read_facet(maths::triangle3d& triangle, maths::vector3d& normal) override;
	bool done() const override;

	size_t get_file_facet_count() override;
};

class stl_importer
{
private:
	std::shared_ptr<std::istream>			m_istream;
	std::unique_ptr<stl_reader_interface>	m_stl_reader;

	std::string								m_stl_name;

	size_t									m_expected_facet_count;
	size_t									m_facets_read;

	std::unique_ptr<stl_reader_interface>	create_stl_reader_();

public:
	stl_importer(const std::shared_ptr<std::istream>& istream);
	stl_importer(const std::string& filename);

	const std::string& name() const { return m_stl_name; }

	/** The number of facets that we expect to read from the input STL */
	size_t num_facets_expected() const { return m_expected_facet_count; }

	/** The number of facets that we actually read from the input STL */
	size_t num_facets_read() const { return m_facets_read; }

	template <typename OutputIterator>
	void import(OutputIterator oi)
	{
		if (!m_stl_reader)
		{
			auto stl_reader = create_stl_reader_();
			if (stl_reader)
				m_stl_reader = std::move(stl_reader);
			else
				throw std::runtime_error("Error creating STL reader!");
		}

		// Seek to beginning of file, reset istream
		m_istream->seekg(0);
		m_facets_read = 0;

		if (!m_stl_reader->read_header(m_stl_name))
			return;	// TODO - throw exception

		while (!m_stl_reader->done())
		{
			maths::triangle3d triangle;
			maths::vector3d normal;

			if (m_stl_reader->read_facet(triangle, normal))
			{
				*oi++ = triangle;

				m_facets_read++;
			}
		}
	}
};

};

#endif //STL_IMPORTER_H_
