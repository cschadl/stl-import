#include <iostream>
#include <iomanip>
#include <sstream>

#include <tut.h>

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::setprecision;
using std::fixed;

class callback : public tut::callback
{
protected:
	unsigned int tests_passed;
	unsigned int tests_failed;

public:
	callback() : tests_passed(0), tests_failed(0) { }

	virtual void run_started()
	{
		cout << "Running all tests..." << endl << endl;
	}

	virtual void group_started(const string& group_name)
	{
		cout << group_name << endl;
	}

	virtual void test_completed(const tut::test_result& tr)
	{
		stringstream result_ss;
		if (tr.result != tut::test_result::ok)
		{
			result_ss << "FAILED";
			tests_failed++;
		}
		else
		{
			result_ss << "OK";
			tests_passed++;
		}

		cout << tr.test << ": " << tr.name << "... " << result_ss.str() << endl;
	}

	virtual void group_completed(const string&)
	{
		cout << endl;
	}

	virtual void run_completed()
	{
		const unsigned int tests_run = tests_failed + tests_passed;
		const float success_percent = 100.0f * ((float) tests_passed / (float) tests_run);
		cout << "Ran " << tests_run << " tests, "
			 << tests_passed << " passed, " << tests_failed << " failed - "
			 << fixed << setprecision(2) << success_percent << "% success" << endl;
	}
};

int main(int argc, char** argv)
{
	callback clbk;
	tut::test_runner_singleton runner;

	runner.get().set_callback(&clbk);
	runner.get().run_tests();

	return 0;
}
