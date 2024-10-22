//============================================================================
// Name        : VectorSorting.cpp
// Author      : Reese Thurman
// Version     : 1.0
// Description : Vector Sorting Algorithms
//============================================================================

#include <algorithm>
#include <iostream>
#include <time.h>
#include <fstream>
#include <sstream>
#include <iomanip>
# include <stdexcept>
# include <string>
# include <vector>
# include <list>
# include <sstream>

using namespace std;

//============================================================================
// Global definitions visible to all methods and classes
//============================================================================

// forward declarations
double strToDouble(string str, char ch);

// define a structure to hold course information
struct Course {
    string courseId; // unique identifier
    string title;
    vector<string> prereqs;
};


namespace csv
{
    class Error : public std::runtime_error
    {

    public:
        Error(const std::string& msg) :
            std::runtime_error(std::string("CSVparser : ").append(msg))
        {
        }
    };

    class Row
    {
    public:
        Row(const std::vector<std::string>&);
        ~Row(void);

    public:
        unsigned int size(void) const;
        void push(const std::string&);
        bool set(const std::string&, const std::string&);

    private:
        const std::vector<std::string> _header;
        std::vector<std::string> _values;

    public:

        template<typename T>
        const T getValue(unsigned int pos) const
        {
            if (pos < _values.size())
            {
                T res;
                std::stringstream ss;
                ss << _values[pos];
                ss >> res;
                return res;
            }
            throw Error("can't return this value (doesn't exist)");
        }
        const std::string operator[](unsigned int) const;
        const std::string operator[](const std::string& valueName) const;
        friend std::ostream& operator<<(std::ostream& os, const Row& row);
        friend std::ofstream& operator<<(std::ofstream& os, const Row& row);
    };

    enum DataType {
        eFILE = 0,
        ePURE = 1
    };

    class Parser
    {

    public:
        Parser(const std::string&, const DataType& type = eFILE, char sep = ',');
        ~Parser(void);

    public:
        Row& getRow(unsigned int row) const;
        unsigned int rowCount(void) const;
        unsigned int columnCount(void) const;
        std::vector<std::string> getHeader(void) const;
        const std::string getHeaderElement(unsigned int pos) const;
        const std::string& getFileName(void) const;

    public:
        bool deleteRow(unsigned int row);
        bool addRow(unsigned int pos, const std::vector<std::string>&);
        void sync(void) const;

    protected:
        void parseHeader(void);
        void parseContent(void);

    private:
        std::string _file;
        const DataType _type;
        const char _sep;
        std::vector<std::string> _originalFile;
        std::vector<std::string> _header;
        std::vector<Row*> _content;

    public:
        Row& operator[](unsigned int row) const;
    };
}

// The following code parses course data from a CSV file
namespace csv {

    Parser::Parser(const std::string& data, const DataType& type, char sep)
        : _type(type), _sep(sep)
    {
        std::string line;
        if (type == eFILE)
        {
            _file = data;
            std::ifstream ifile(_file.c_str());
            if (ifile.is_open())
            {
                while (ifile.good())
                {
                    getline(ifile, line);
                    if (line != "")
                        _originalFile.push_back(line);
                }
                ifile.close();

                if (_originalFile.size() == 0)
                    throw Error(std::string("No Data in ").append(_file));

                parseHeader();
                parseContent();
            }
            else
                throw Error(std::string("Failed to open ").append(_file));
        }
        else
        {
            std::istringstream stream(data);
            while (std::getline(stream, line))
                if (line != "")
                    _originalFile.push_back(line);
            if (_originalFile.size() == 0)
                throw Error(std::string("No Data in pure content"));

            parseHeader();
            parseContent();
        }
    }

    Parser::~Parser(void)
    {
        std::vector<Row*>::iterator it;

        for (it = _content.begin(); it != _content.end(); it++)
            delete* it;
    }

    void Parser::parseHeader(void)
    {
        std::stringstream ss(_originalFile[0]);
        std::string item;

        while (std::getline(ss, item, _sep))
            _header.push_back(item);
    }

    void Parser::parseContent(void)
    {
        std::vector<std::string>::iterator it;

        //vector<string> courseData;
        

        it = _originalFile.begin();
        //it++; // skip header

        for (; it != _originalFile.end(); it++)
        {
            bool quoted = false;
            int tokenStart = 0;
            unsigned int i = 0;

            Row* row = new Row(_header);

            for (; i != it->length(); i++)
            {
                if (it->at(i) == '"')
                    quoted = ((quoted) ? (false) : (true));
                else if (it->at(i) == ',' && !quoted)
                {
                    row->push(it->substr(tokenStart, i - tokenStart));
                    tokenStart = i + 1;
                }
            }

            //end
            row->push(it->substr(tokenStart, it->length() - tokenStart));

            /* // if value(s) missing
            if (row->size() != _header.size())
                throw Error("corrupted data !"); */
            _content.push_back(row);
        }
    }

    Row& Parser::getRow(unsigned int rowPosition) const
    {
        if (rowPosition < _content.size())
            return *(_content[rowPosition]);
        throw Error("can't return this row (doesn't exist)");
    }

    Row& Parser::operator[](unsigned int rowPosition) const
    {
        return Parser::getRow(rowPosition);
    }

    unsigned int Parser::rowCount(void) const
    {
        return _content.size();
    }

    unsigned int Parser::columnCount(void) const
    {
        return _header.size();
    }

    std::vector<std::string> Parser::getHeader(void) const
    {
        return _header;
    }

    const std::string Parser::getHeaderElement(unsigned int pos) const
    {
        if (pos >= _header.size())
            throw Error("can't return this header (doesn't exist)");
        return _header[pos];
    }

    bool Parser::deleteRow(unsigned int pos)
    {
        if (pos < _content.size())
        {
            delete* (_content.begin() + pos);
            _content.erase(_content.begin() + pos);
            return true;
        }
        return false;
    }

    bool Parser::addRow(unsigned int pos, const std::vector<std::string>& r)
    {
        Row* row = new Row(_header);

        for (auto it = r.begin(); it != r.end(); it++)
            row->push(*it);

        if (pos <= _content.size())
        {
            _content.insert(_content.begin() + pos, row);
            return true;
        }
        return false;
    }

    void Parser::sync(void) const
    {
        if (_type == DataType::eFILE)
        {
            std::ofstream f;
            f.open(_file, std::ios::out | std::ios::trunc);

            // header
            unsigned int i = 0;
            for (auto it = _header.begin(); it != _header.end(); it++)
            {
                f << *it;
                if (i < _header.size() - 1)
                    f << ",";
                else
                    f << std::endl;
                i++;
            }

            for (auto it = _content.begin(); it != _content.end(); it++)
                f << **it << std::endl;
            f.close();
        }
    }

    const std::string& Parser::getFileName(void) const
    {
        return _file;
    }

    /*
    ** ROW
    */

    Row::Row(const std::vector<std::string>& header)
        : _header(header) {}

    Row::~Row(void) {}

    unsigned int Row::size(void) const
    {
        return _values.size();
    }

    void Row::push(const std::string& value)
    {
        _values.push_back(value);
    }

    bool Row::set(const std::string& key, const std::string& value)
    {
        std::vector<std::string>::const_iterator it;
        int pos = 0;

        for (it = _header.begin(); it != _header.end(); it++)
        {
            if (key == *it)
            {
                _values[pos] = value;
                return true;
            }
            pos++;
        }
        return false;
    }

    const std::string Row::operator[](unsigned int valuePosition) const
    {
        if (valuePosition < _values.size())
            return _values[valuePosition];
        throw Error("can't return this value (doesn't exist)");
    }

    const std::string Row::operator[](const std::string& key) const
    {
        std::vector<std::string>::const_iterator it;
        int pos = 0;

        for (it = _header.begin(); it != _header.end(); it++)
        {
            if (key == *it)
                return _values[pos];
            pos++;
        }

        throw Error("can't return this value (doesn't exist)");
    }

    std::ostream& operator<<(std::ostream& os, const Row& row)
    {
        for (unsigned int i = 0; i != row._values.size(); i++)
            os << row._values[i] << " | ";

        return os;
    }

    std::ofstream& operator<<(std::ofstream& os, const Row& row)
    {
        for (unsigned int i = 0; i != row._values.size(); i++)
        {
            os << row._values[i];
            if (i < row._values.size() - 1)
                os << ",";
        }
        return os;
    }
}


//============================================================================
// Static methods used for testing
//============================================================================

/**
 * Display the course information to the console (std::out)
 *
 * @param course struct containing the course info
 */
void displayCourse(Course course) {
    cout << course.courseId << ", " << course.title << endl;
    return;
}

/**
 * Prompt user for course information using console (std::in)
 *
 * @return Course struct containing the course info
 */
void getCourse(vector<Course> courses) {
    string courseId;
    Course course;
    bool isFound = false;

    cout << "What course do you want to know about? ";
    cin >> courseId;

    transform(courseId.begin(), courseId.end(), courseId.begin(), ::toupper);

    for (int i = 0; i < courses.size(); i++) {

        if (courses[i].courseId == courseId) {
            isFound = true;
            cout << courses[i].courseId << ", " << courses[i].title << endl;

            cout << "Prerequisites: ";

            for (int i2 = 0; i2 < courses[i].prereqs.size(); i2++) {

                if (i2 == (courses[i].prereqs.size() - 1))
                    cout << courses[i].prereqs[i2];

                else
                    cout << courses[i].prereqs[i2] << ", ";


            }

        }
            
    }

    cout << endl;

    if (isFound == false)
        cout << "Course not found." << endl;
}

/**
 * Load a CSV file containing courses into a container
 *
 * @param csvPath the path to the CSV file to load
 * @return a container holding all the courses read
 */
vector<Course> loadCourses(string csvPath) {

    int i2 = 2;

    cout << "Loading CSV file " << csvPath << endl;

    // Define a vector data structure to hold a collection of courses.
    vector<Course> courses;

    try {

        // initialize the CSV Parser using the given path
        csv::Parser file = csv::Parser(csvPath);

        // loop to read rows of a CSV file
        for (int i = 0; i < file.rowCount(); i++) {

            // Create a data structure and add to the collection of courses
            Course course;
            course.courseId = file[i][0];
            course.title = file[i][1];

            while (i2 < file[i].size()) {
                if (file[i][i2] != "")
                    course.prereqs.push_back(file[i][i2]);
                i2++;
            }

            i2 = 2;

            //cout << "Item: " << course.title << ", Fund: " << course.fund << ", Amount: " << course.amount << endl;

            // push this course to the end
            courses.push_back(course);
        }
    }
    catch (csv::Error& e) {
        std::cerr << e.what() << std::endl;
    }
    return courses;
}

/**
 * Partition the vector of courses into two parts, low and high
 *
 * @param courses Address of the vector<Course> instance to be partitioned
 * @param begin Beginning index to partition
 * @param end Ending index to partition
 */
int partition(vector<Course>& courses, int end, int begin) {

    // Calculate the middle element as middlePoint (int)
    // Set Pivot as middlePoint element title to compare (string)  
    // Pick middle element and assign the value to pivot
    int midPoint = end + (begin - end) / 2;
    string pivot = courses[midPoint].title;
    string sTemp;

    bool isDone = false;

    while (isDone == false) {
        // Increment end while courses[end] < pivot
        while (courses[end].title < pivot) {
            end += 1;
        }

        // Decrement begin while pivot < courses[begin]
        while (pivot < courses[begin].title) {
            begin -= 1;
        }

        // If zero or one elements remain, then all numbers are 
        // partitioned. Return begin.
        if (end >= begin) {
            isDone = true;
        }
        else {
            // Swap courses[end] and courses[begin]
            sTemp = courses[end].title;
            courses[end].title = courses[begin].title;
            courses[begin].title = sTemp;

            // Update begin and end
            end += 1;
            begin -= 1;
        }
    }

    return begin;

}

/**
 * Perform a selection sort on course title
 * Average performance: O(n^2))
 * Worst case performance O(n^2))
 *
 * @param course address of the vector<Course>
 *            instance to be sorted
 */
void selectionSort(vector<Course>& courses) {

    int iSmallest = 0;
    Course cTemp;              // Temporary variable for swap

    for (int i = 0; (i < courses.size() - 1); ++i) {

        // Find index of smallest remaining element
        iSmallest = i;

        // loop over elements right of position
        // if the element title is less than minimum title
        // this element becomes the new minimum
        for (int i2 = i + 1; (i2 < courses.size()); ++i2) {
            if (courses[i2].courseId < courses[iSmallest].courseId) {
                iSmallest = i2;
            }
        }

        // Swap courses[i] and courses[iSmallest]
        cTemp = courses[i];
        courses[i] = courses[iSmallest];
        courses[iSmallest] = cTemp;
    }
}

/**
 * Simple C function to convert a string to a double
 * after stripping out unwanted char
 *
 * credit: http://stackoverflow.com/a/24875936
 *
 * @param ch The character to strip out
 */
double strToDouble(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return atof(str.c_str());
}

/**
 * The one and only main() method
 */
int main(int argc, char* argv[]) {

    // process command line arguments
    string csvPath;

    // Define a vector to hold all the courses
    vector<Course> courses;

    // Define a timer variable
    clock_t ticks;

    int choice = 0;
    while (choice != 9) {
        cout << "Welcome to the course planner." << endl;
        cout << "  1. Load Courses" << endl;
        cout << "  2. Print Course List" << endl;
        cout << "  3. Search for a Course" << endl;
        cout << "  9. Exit" << endl;
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {

        case 1:

            cout << "Enter filename for course data: ";
            cin.ignore();
            getline(cin, csvPath);


            // Complete the method call to load the courses
            courses = loadCourses(csvPath);

            cout << courses.size() << " courses read" << endl;



            break;

        case 2:

            if (courses.size() > 0)
                selectionSort(courses);

            // Loop and display the courses read
            for (int i = 0; i < courses.size(); ++i) {
                displayCourse(courses[i]);
            }
            cout << endl;

            break;

        case 3:
            
            getCourse(courses);


            break;

        }

        if (choice != 1 && choice != 2 && choice != 3 && choice != 9)
            cout << choice << " is not a valid option." << endl;
    }

    cout << "Thank you for using the course planner!" << endl;

    return 0;
}
