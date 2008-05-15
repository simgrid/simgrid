

#ifndef MSG_OBJECT_H
#define MSG_OBJECT_H

// Compilation C++ recquise
#ifndef __cplusplus
	#error Object.hpp requires C++ compilation (use a .cxx suffix)
#endif

#include <stdlib.h>

namespace msg
{
		//////////////////////////////////////////////////////////////////////////////
		// Macros
		
		// Returns the runtime class of the class_name object.
		#define MSG_GET_CLASS(class_name) \
		 			((Class*)(&class_name::class##class_name))
			
		// Declare the class class_name as dynamic
		#define MSG_DECLARE_DYNAMIC(class_name) \
		public: \
			static const Class class##class_name; \
			virtual Class* getClass() const; \
			static Object*  createObject(); \

		// The runtime class implementation.	
		#define MSG_IMPLEMENT_CLASS(class_name, base_class_name, pfn,class_init) \
			const Class class_name::class##class_name = { \
				#class_name, sizeof(class class_name),pfn, \
					MSG_GET_CLASS(base_class_name), NULL,class_init}; \
			Class* class_name::getClass() const \
                                { return MSG_GET_CLASS(class_name); } \
		
		// CreateObject implementation.	
		#define MSG_IMPLEMENT_DYNAMIC(class_name, base_class_name) \
			Object* class_name::createObject() \
				{ return (Object*)(new class_name); } \
			DeclaringClass _declaringClass_##class_name(MSG_GET_CLASS(class_name)); \
			MSG_IMPLEMENT_CLASS(class_name, base_class_name, \
				class_name::createObject, &_declaringClass_##class_name) \


		//////////////////////////////////////////////////////////////////////////////
		// Classes declared in this file.

		class Object;	// The msg object.

		//////////////////////////////////////////////////////////////////////////////
		// Structures declared in this files.

		struct Class;	        // used during the rtti operations
		struct DeclaringClass;	// used during the instances registration.


        class DeclaringClasses;

		//////////////////////////////////////////////////////////////////////////////
		// Global functions

		// Used during the registration.
		void DeclareClass(Class* c);

		//////////////////////////////////////////////////////////////////////////////
		// DeclaringClass

		struct DeclaringClass
		{
			// Constructor : add the runtime classe in the list.
			DeclaringClass(Class* c);
			
			// Destructor
			virtual ~DeclaringClass(void);

		// Attributes :
		    // the list of runtime classes.
		    static DeclaringClasses* declaringClasses;
		};

        #define MSG_DELCARING_CLASSES (*(DeclaringClass::declaringClasses))
		
		
		struct Class
		{
		
		// Attributes
	
			const char* name;							// class name.
			int typeSize;								// type size.			
			Object* (*createObjectFn)(void);				// pointer to the create object function. 
			Class* baseClass;						// the runtime class of the runtime class.
			Class* next;						// the next runtime class in the list.
			const DeclaringClass* declaringClass;	// used during the registration of the class.
		
		// Operations
		
			// Create the runtime class from its name.
			static Class* fromName(const char* name);
			
			// Create an object from the name of the its class.
			static Object* createObject(const char* name);
			
			// Create an instance of the class.
			Object* createObject(void);
			
			// Return true is the class is dervived from the base class baseClass.
			bool isDerivedFrom(const Class* baseClass) const;

		};

        // Create an instance of the class.
        inline Object* Class::createObject(void)
        {
            return (*createObjectFn)();
        }
	
            
		class Object
		{
		public:
		
			// Default constructor.
			Object(){}
			
			// Destructor.
			virtual ~Object(){}
			
			// Operations.
		
			// Get the runtime class.
			virtual Class* getClass(void) const;
			
			// Returns true if the class is derived from the class baseClass. Otherwise
			// the method returns false.
			bool isDerivedFrom(const Class* baseClass) const;
			
			// Returns true if the object is valid. Otherwise the method returns false.
			virtual bool isValid(void) const;
			
			// Operators.

			// Attributes.
			
			// The runtime class.
			static const Class classObject;
		};

		// inline member functions of the class Object.
		
		// Returns the runtime class of the object.
		inline Class* Object::getClass(void) const
		{
			return MSG_GET_CLASS(Object);
		}

        // Returns true if the class is derived from the class pBaseClass. Otherwise
		// the method returns false.
        inline bool Object::isDerivedFrom(const Class* baseClass) const
        {
            return (getClass()->isDerivedFrom(baseClass));
        }
		
		// Returns true if the object is valid. Otherwise the method returns false.
		inline bool Object::isValid(void) const
		{
			// returns always true.
			return true;	
		}
	
		class DeclaringClasses 
		{
		public:
		
			// Constructor.
			DeclaringClasses();

			// Destructor.
			virtual ~DeclaringClasses(){}
		
		// Operations.
		
			// Add the class at the head of the list.
			void addHead(Class* c);
			
			// Get the runtime class of the head of the list.
			Class* getHead(void) const ;
			
			// Remove the class from the list (don't destroy it).
			bool remove(Class* c);
            
            // Remove the head of the list.
			Class* removeHead(void);
			
			// Return true if the list is empty.
			
			bool isEmpty(void) const;
			
			// Remove of the elements of the list.
			void removeAll(void);
			
			// Get the number of classes in the list.
		    unsigned int getCount(void);
		    
		    void lock(void){}
		    
		    void unlock(void){}
		
			//Attributes
		
			// The head of the list.
			Class* head;

		private:
		
		// Attributes
		
			// The number of elements of the list.
			unsigned int count;
		};


        // Constructor (Add the class in the list).
        inline DeclaringClass::DeclaringClass(Class* c)
        {
                if(!declaringClasses)
                        declaringClasses = new DeclaringClasses();

                DeclareClass(c);
        }

        // Destructor.
        inline DeclaringClass::~DeclaringClass()
        {
                /*if(NULL != declaringClasses)
                        delete declaringClasses;

                declaringClasses=NULL;*/

        }

		// Returns the number of elements of the list.
		inline unsigned int DeclaringClasses::getCount()
		{
			return count;
		}
		
		// Returns the head of the list.
		inline Class* DeclaringClasses::getHead() const
		{
			return head;
		}
		
		// Returns true if the list is empty. Otherwise this function
		// returns false.
		inline bool DeclaringClasses::isEmpty() const
		{
			return(!head);
		}
		
		// Removes all the elements of the list.
		inline void DeclaringClasses::removeAll()
		{
			head = 0;
		    count=0;
		}
			
       
} 

using namespace msg;

#endif // !MSG_OBJECT_H

