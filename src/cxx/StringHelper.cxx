#include <StringHelper.hpp>

#include <BadAllocException.hpp>
#include <NullPointerException.hpp>
#include <InvalidArgumentException.hpp>
#include <OutOfBoundsException.hpp>

#ifndef BUFF_MAX
#define BUFF_MAX ((size_t)260)
#endif // BUFF_MAX


// namespace SimGrid::Msg
namespace SimGrid
{
	namespace Msg
	{

		#define DEFAULT_STRING_HELPER_CAPACITY ((int)128)
		

		void StringHelper::init(void)
		{
			capacity = DEFAULT_STRING_HELPER_CAPACITY;

			if(!(content = (char*) calloc(capacity + 1, sizeof(char))))
				throw BadAllocException();

			len = 0;
		}
				
	// Default constructor
		StringHelper::StringHelper()
		{
			init();
		}

		StringHelper::StringHelper(char c)
		{
			init();
			append(c);

		}
		
	 	StringHelper::StringHelper(char c, int n)
		{
			init();
			append(c, n);
		}
		
		StringHelper::StringHelper(const char* cstr)
		{
			init();
			append(cstr);
		}
		
		StringHelper::StringHelper(const char* cstr, int n)
		{
			init();
			append(cstr, n);
		}
		
		StringHelper::StringHelper(const char* cstr, int pos, int n)
		{
			init();
			append(cstr, pos, n);
		}
		
		StringHelper::StringHelper(const string& rstr)
		{
			init();
			append(rstr);
		}
		
		StringHelper::StringHelper(const string& rstr, int n)
		{
			init();
			append(rstr, n);
		}
		
		StringHelper::StringHelper(const string& rstr, int pos, int n)
		{
			init();
			append(rstr, pos, n);
		}
		
		StringHelper::StringHelper(short si)
		{
			init();
			append(si);
		}
		
		StringHelper::StringHelper(int i)
		{
			init();
			append(i);
		}
		
		StringHelper::StringHelper(long l)
		{
			init();
			append(l);
		}
		
		StringHelper::StringHelper(float f)
		{
			init();
			append(f);
		}
		
		StringHelper::StringHelper(double d)
		{
			init();
			append(d);
		}

		StringHelper::StringHelper(double d, const char* format)
		{
			char toAppend[BUFF_MAX + 1] = {0};
			
			sprintf(toAppend,format,d);

			init();

			append(toAppend);
		}
		
		StringHelper::StringHelper(unsigned short usi)
		{
			init();
			append(usi);
		}
		
		StringHelper::StringHelper(unsigned int ui)
		{
			init();
			append(ui);
		}
		
		StringHelper::StringHelper(unsigned long ul)
		{
			init();
			append(ul);
		}
	
	// Copy constructor
		StringHelper::StringHelper(const StringHelper& rStringHelper)
		{
			if(this != &rStringHelper && rStringHelper.size())
			{
				clear();
				append(rStringHelper.cstr());
			}
		}

	// Destructor
		StringHelper::~StringHelper()
		{
			if(content)
				free(content);
		}

	// Operations

		void StringHelper::clear(void)
		{
			if(len)
				memset(content, 0, len);

			len = 0;
		}

		bool StringHelper::empty(void)
		{
			return len == 0;
		}
	
		StringHelper& StringHelper::append(unsigned char c)
		{
			if(capacity < len + 1)
			{
				int new_capacity = (capacity << 1) ;

				if(!(content = (char*) realloc(content, new_capacity)))
					throw BadAllocException();

				capacity = new_capacity;
			}
			

			content[len] = c;
			len++;

			content[len] = '\0';

			return *this;
		}

	 	StringHelper& StringHelper::append(unsigned char c, int n)
	 	{
			if(n <=0)
				throw InvalidArgumentException("n");

			char* toAppend = (char*) calloc(n + 1, sizeof(char));

			if(!toAppend)
				throw BadAllocException();

			memset(toAppend, c, n); 

			append(toAppend);

			free(toAppend);

			return *this;
		}

	
		StringHelper& StringHelper::append(char c)
		{
			if(capacity < len + 1)
			{
				int new_capacity = (capacity << 1) ;

				if(!(content = (char*) realloc(content, new_capacity)))
					throw BadAllocException();

				capacity = new_capacity;
			}
			

			content[len] = c;
			len++;

			content[len] = '\0';

			return *this;
		}
		
	 	StringHelper& StringHelper::append(char c, int n)
		{
			if(n <=0)
				throw InvalidArgumentException("n");

			char* toAppend = (char*) calloc(n + 1, sizeof(char));

			if(!toAppend)
				throw BadAllocException();

			memset(toAppend, c, n); 

			append(toAppend);

			free(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(const char* cstr)
		{
			if(!cstr)
				throw NullPointerException("cstr");
			
			int l =  (int) strlen(cstr);

			if(capacity < len + l)
			{
				int new_capacity = (capacity << 1) < (len + l) ? (len + l) << 1 : capacity << 1;

				if(!(content = (char*) realloc(content, new_capacity)))
					throw BadAllocException();

				strcat(content, cstr);

				capacity = new_capacity;
			}
			else
			{
				strcat(content, cstr);
			}

			len += l;
			content[len] = '\0';

			return *this;
			
		}
		
		StringHelper& StringHelper::append(const char* cstr, int n)
		{
			if(!cstr)
				throw NullPointerException("cstr");

			if(n <= 0)
				throw InvalidArgumentException("n");

			
			int l =  ((int) strlen(cstr)) * n;

			if(capacity < len + l)
			{
				int new_capacity = (capacity << 1) < (len + l) ? (len + l) << 1 : capacity << 1;

				if(!(content = (char*) realloc(content, new_capacity)))
					throw BadAllocException();
				
				for(int i = 0; i < n; i++)
					strcat(content, cstr);

				capacity = new_capacity;
			}
			else
			{
				for(int i = 0; i < n; i++)
					strcat(content, cstr);
			}

			len += l;
			content[len] = '\0';

			return *this;

		}
		
		StringHelper& StringHelper::append(const char* cstr, int pos, int n)
		{
			if(!cstr)
				throw NullPointerException("cstr");

			if(n <= 0)
				throw InvalidArgumentException("n");

			if(pos < 0 || pos >= (int)strlen(cstr) )
				throw OutOfBoundsException(pos);

			if(pos + n >= (int)strlen(cstr))
				throw OutOfBoundsException(pos, n);


			char* toAppend = (char*) calloc(n + 1, sizeof(char));
			
			strncpy(toAppend, cstr + pos, n);

			append(toAppend);

			free(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(const string& rstr)
		{
			append(rstr.c_str());
			return *this;
		}
		
		StringHelper& StringHelper::append(const string& rstr, int n)
		{
			if(rstr.empty())
				throw NullPointerException("rstr");

			if(n <= 0)
				throw InvalidArgumentException("n");

			
			int l =  ((int) rstr.size()) * n;

			if(capacity < len + l)
			{
				int new_capacity = (capacity << 1) < (len + l) ? (len + l) << 1 : capacity << 1;

				if(!(content = (char*) realloc(content, new_capacity)))
					throw BadAllocException();
				
				for(int i = 0; i < n; i++)
					strcat(content, rstr.c_str());

				capacity = new_capacity;
			}
			else
			{
				for(int i = 0; i < n; i++)
					strcat(content, rstr.c_str());
			}

			len += l;
			content[len] = '\0';

			return *this;
		}
		
		StringHelper& StringHelper::append(const string& rstr, int pos, int n)
		{
			if(rstr.empty())
				throw InvalidArgumentException("rstr");

			if(n <= 0)
				throw InvalidArgumentException("n");

			if(pos < 0 || pos >= (int) rstr.size() )
				throw OutOfBoundsException(pos);

			if(pos + n >=  (int) rstr.size())
				throw OutOfBoundsException(pos, n);


			char* toAppend = (char*) calloc(n + 1, sizeof(char));
			
			strncpy(toAppend, rstr.c_str() + pos, n);

			append(toAppend);

			free(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(short si)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%hd",si);

			append(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(int i)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%d",i);

			append(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(long l)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%ld",l);

			append(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(float f)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%f",f);

			append(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(double d)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%#7lf",d);

			append(toAppend);

			return *this;
		}

		StringHelper& StringHelper::append(double d, const char* format)
		{
			char toAppend[BUFF_MAX + 1] = {0};

			sprintf(toAppend, format, d);

			append(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(unsigned short usi)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%hu",usi);

			append(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(unsigned int ui)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%u",ui);

			append(toAppend);

			return *this;
		}
		
		StringHelper& StringHelper::append(unsigned long ul)
		{
			char toAppend[26] = {0};

			sprintf(toAppend, "%lu",ul);

			append(toAppend);

			return *this;
		}
			
		const char& StringHelper::at(int pos) const
		{
			if(pos < 0 || pos >= len)
				throw OutOfBoundsException(pos);
			
			return content[pos];
		}
		
		char& StringHelper::at(int pos)
		{
			if(pos < 0 || pos >= len)
				throw OutOfBoundsException(pos);
			
			return content[pos];
		}
		
		const char* StringHelper::cstr(void) const
		{
			return (const char*)content;
		}
		
		string& StringHelper::toString(void)
		{
			string* s = new string();
			s->append(content);
			return *s;
		}
		
		int StringHelper::size(void) const
		{
			return len;
		}
		
	//  Operators
		
		// Assignement
		StringHelper& StringHelper::operator = (const StringHelper& rStringHelper)
		{

			if(this !=&rStringHelper && rStringHelper.size())
			{
				clear();
				append(rStringHelper.cstr());
			}

			return *this;
		}
		
		StringHelper& StringHelper::operator = (const char* cstr)
		{
			clear();
			append(cstr);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (const string& str)
		{
			clear();
			append(str);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (short n)
		{
			clear();
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (int n)
		{
			clear();
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (long n)
		{
			clear();
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (float n)
		{
			clear();
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (double n)
		{
			clear();
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (unsigned short n)
		{
			clear();
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (unsigned int n)
		{
			clear();
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator = (unsigned long n)
		{
			clear();
			append(n);
			return *this;
		}
		
		char& StringHelper::operator[](int pos)
		{
			if(pos < 0 || pos >= len)
				throw OutOfBoundsException(pos);
			
			return content[pos];	
		}
		
		char StringHelper::operator[](int pos) const
		{
			if(pos < 0 || pos >= len)
				throw OutOfBoundsException(pos);
			
			return content[pos];
		}
		
		StringHelper& StringHelper::operator += (short n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (int n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (long n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (float n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (double n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (unsigned short n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (unsigned int n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (unsigned long n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (const StringHelper& rStringHelper)
		{
			append(rStringHelper.content);
			return *this;
		}

		StringHelper& StringHelper::operator += (const string& rstr)
		{
			append(rstr.c_str());
			return *this;
		}
		
		StringHelper& StringHelper::operator += (const char* cstr)
		{
			append(cstr);
			return *this;
		}
		
		StringHelper& StringHelper::operator += (char c)
		{
			append(c);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (short n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (int n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (long n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (float n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (double n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (unsigned short n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (unsigned int n)
		{
			append(n);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (unsigned long n)
		{
			append(n);
			return *this;
		}
		
		
		StringHelper& StringHelper::operator + (const StringHelper& rStringHelper)
		{
			append(rStringHelper.content);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (const string& rstr)
		{
			append(rstr.c_str());
			return *this;
		}
		
		StringHelper& StringHelper::operator + (const char* cstr)
		{
			append(cstr);
			return *this;
		}
		
		StringHelper& StringHelper::operator + (char c)
		{
			append(c);
			return *this;
		}

		StringHelper::operator char *()
		{
			return content;
		}

		StringHelper::operator const char *()
		{
			return content;
		}

		ostream& operator<<(ostream& stream, const StringHelper& s)
		{
			stream << s.cstr();
			return stream;
		}

		istream& operator<<(istream& stream, StringHelper& s)
		{
			char buff[256] = {0};

			stream >> buff;

			s.append(buff);

			return stream;
		}
				
	} // namespace Msg
} // namespace SimGrid

