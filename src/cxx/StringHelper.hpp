/*
 * StringHelper.hpp
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 *
 */  
 
#ifndef STRING_HELPER_HPP
#define STRING_HELPER_HPP

 #ifndef __cplusplus
	#error StringHelper.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <string>
using std::string;

#include<iostream>
using std::ostream;
using std::istream;

#include <Config.hpp>

// namespace SimGrid::Msg
namespace SimGrid
{
	namespace Msg
	{
		class SIMGRIDX_EXPORT StringHelper
		{
			public:
				
				// Default constructor
					StringHelper();

					StringHelper(unsigned char c);
				
				 	StringHelper(unsigned char c, int n);
				
					StringHelper(char c);
				
				 	StringHelper(char c, int n);
				
					StringHelper(const char* cstr);
					
					StringHelper(const char* cstr, int n);
					
					StringHelper(const char* cstr, int pos, int n);
					
					StringHelper(const string& rstr);
					
					StringHelper(const string& rstr, int n);
					
					StringHelper(const string& rstr, int pos, int n);
					
					StringHelper(short si);
					
					StringHelper(int i);
					
					StringHelper(long l);
					
					StringHelper(float f);
					
					StringHelper(double d);

					StringHelper(double d, const char* format);
					
					StringHelper(unsigned short usi);
					
					StringHelper(unsigned int ui);
					
					StringHelper(unsigned long ul);
				
				// Copy constructor
					StringHelper(const StringHelper& rStringHelper);
				
				// Destructor
					~StringHelper();

				// Operations
				
					StringHelper& append(unsigned char c);
				
				 	StringHelper& append(unsigned char c, int n);
				
					StringHelper& append(char c);
				
				 	StringHelper& append(char c, int n);
				
					StringHelper& append(const char* cstr);
					
					StringHelper& append(const char* cstr, int n);
					
					StringHelper& append(const char* cstr, int pos, int n);
					
					StringHelper& append(const string& rstr);
					
					StringHelper& append(const string& rstr, int n);
					
					StringHelper& append(const string& rstr, int pos, int n);
					
					StringHelper& append(short si);
					
					StringHelper& append(int i);
					
					StringHelper& append(long l);
					
					StringHelper& append(float f);
					
					StringHelper& append(double d);
					
					StringHelper& append(double d, const char* format);
					
					StringHelper& append(unsigned short usi);
					
					StringHelper& append(unsigned int ui);
					
					StringHelper& append(unsigned long ul);
					
    				const char& at(int pos) const;
    				
    				char& at(int pos);
					
					const char* cstr(void) const;
					
					string& toString(void);
					
					int size(void) const;

					void clear(void);

					bool empty(void);
					
				//  Operators
					
					// Assignement
					StringHelper& operator = (const StringHelper& rStringHelper);
					
					StringHelper& operator = (const char* cstr);
					
					StringHelper& operator = (const string& str);
					
					StringHelper& operator = (short n);
					
					StringHelper& operator = (int n);
					
					StringHelper& operator = (long n);
					
					StringHelper& operator = (float n);
					
					StringHelper& operator = (double n);
					
					StringHelper& operator = (unsigned short n);
					
					StringHelper& operator = (unsigned int n);
					
					StringHelper& operator = (unsigned long n);
					
					char& operator[](int pos);
					
					char operator[](int pos) const;
					
					StringHelper& operator += (short n);
					
					StringHelper& operator += (int n);
					
					StringHelper& operator += (long n);
					
					StringHelper& operator += (float n);
					
					StringHelper& operator += (double n);
					
					StringHelper& operator += (unsigned short n);
					
					StringHelper& operator += (unsigned int n);
					
					StringHelper& operator += (unsigned long n);
					
					StringHelper& operator += (const StringHelper& rStringHelper);
					
					StringHelper& operator += (const string& rstr);
					
    				StringHelper& operator += (const char* cstr);
    				
    				StringHelper& operator += (char c);
    				
					StringHelper& operator + (short n);
					
					StringHelper& operator + (int n);
					
					StringHelper& operator + (long n);
					
					StringHelper& operator + (float n);
					
					StringHelper& operator + (double n);
					
					StringHelper& operator + (unsigned short n);
					
					StringHelper& operator + (unsigned int n);
					
					StringHelper& operator + (unsigned long n);
					
					StringHelper& operator + (const StringHelper& rStringHelper);
					
					StringHelper& operator + (const string& rstr);
					
    				StringHelper& operator + (const char* cstr);
    				
    				StringHelper& operator + (char c);

					operator char *();

					operator const char *();

					friend SIMGRIDX_EXPORT ostream& operator<<(ostream& stream, const StringHelper& s);

					friend SIMGRIDX_EXPORT istream& operator<<(istream& stream, StringHelper& s);
				
				private:

					// Methods
					void init(void);
					
					// Attributes
					
					char* content;
					int capacity;
					int len;
		};
		
		typedef class StringHelper* StringHelperPtr;

		#define TEXT_(___x)	StringHelper((___x))


	} // namespace Msg
} // namespace SimGrid


#endif // !STRING_HELPER_HPP