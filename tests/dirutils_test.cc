#include <iostream>
#include <platform/dirutils.h>
#include <cstdlib>
#include <list>
#include <string>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#include <direct.h>
static bool CreateDirectory(const std::string &dir) {
    if (_mkdir(dir.c_str()) != 0) {
        return false;
    }
    return true;
}

#define PATH_SEPARATOR "\\"

#else
static bool CreateDirectory(const std::string &dir) {
   if (mkdir(dir.c_str(), S_IXUSR | S_IWUSR | S_IRUSR) != 0) {
      return false;
   }
   return true;
}

#define PATH_SEPARATOR "/"

#endif

#undef NDEBUG
#include <cassert>

int exit_value = EXIT_SUCCESS;

static void expect(const std::string &exp, const std::string &val) {
   if (exp != val) {
      std::cerr << "Expected " << exp << " got [" << val << "]" << std::endl;
      exit_value = EXIT_FAILURE;
   }
}

static void expect(size_t size, const std::vector<std::string> &vec) {
   if (vec.size() != size) {
      std::cerr << "Expected vector of " << size
                << " elements got [" << vec.size() << "]" << std::endl;
      exit_value = EXIT_FAILURE;
   }
}

static void contains(const std::string &val, const std::vector<std::string> &vec) {
   std::vector<std::string>::const_iterator ii;
   for (ii = vec.begin(); ii != vec.end(); ++ii) {
      if (val == *ii) {
         return ;
      }
   }

   std::cerr << "Expected vector to contain [" << val << "]" << std::endl;
   for (ii = vec.begin(); ii != vec.end(); ++ii) {
      std::cerr << "  -> " << *ii << std::endl;
   }
   std::cerr << std::endl;
   exit_value = EXIT_FAILURE;
}

static std::list<std::string> vfs;

static bool exists(const std::string &path) {
   struct stat st;
   return stat(path.c_str(), &st) == 0;
}

static void testDirname(void) {
   // Check the simple case
   expect("foo", CouchbaseDirectoryUtilities::dirname("foo\\bar"));
   expect("foo", CouchbaseDirectoryUtilities::dirname("foo/bar"));

   // Make sure that we remove double an empty chunk
   expect("foo", CouchbaseDirectoryUtilities::dirname("foo\\\\bar"));
   expect("foo", CouchbaseDirectoryUtilities::dirname("foo//bar"));

   // Make sure that we handle the case without a directory
   expect(".", CouchbaseDirectoryUtilities::dirname("bar"));
   expect(".", CouchbaseDirectoryUtilities::dirname(""));

   // Absolute directories
   expect("\\", CouchbaseDirectoryUtilities::dirname("\\bar"));
   expect("\\", CouchbaseDirectoryUtilities::dirname("\\\\bar"));
   expect("/", CouchbaseDirectoryUtilities::dirname("/bar"));
   expect("/", CouchbaseDirectoryUtilities::dirname("//bar"));

   // Test that we work with multiple directories
   expect("1/2/3/4/5", CouchbaseDirectoryUtilities::dirname("1/2/3/4/5/6"));
   expect("1\\2\\3\\4\\5", CouchbaseDirectoryUtilities::dirname("1\\2\\3\\4\\5\\6"));
   expect("1/2\\4/5", CouchbaseDirectoryUtilities::dirname("1/2\\4/5\\6"));
}

static void testBasename(void) {
   expect("bar", CouchbaseDirectoryUtilities::basename("foo\\bar"));
   expect("bar", CouchbaseDirectoryUtilities::basename("foo/bar"));
   expect("bar", CouchbaseDirectoryUtilities::basename("foo\\\\bar"));
   expect("bar", CouchbaseDirectoryUtilities::basename("foo//bar"));
   expect("bar", CouchbaseDirectoryUtilities::basename("bar"));
   expect("", CouchbaseDirectoryUtilities::basename(""));
   expect("bar", CouchbaseDirectoryUtilities::basename("\\bar"));
   expect("bar", CouchbaseDirectoryUtilities::basename("\\\\bar"));
   expect("bar", CouchbaseDirectoryUtilities::basename("/bar"));
   expect("bar", CouchbaseDirectoryUtilities::basename("//bar"));
   expect("6", CouchbaseDirectoryUtilities::basename("1/2/3/4/5/6"));
   expect("6", CouchbaseDirectoryUtilities::basename("1\\2\\3\\4\\5\\6"));
   expect("6", CouchbaseDirectoryUtilities::basename("1/2\\4/5\\6"));
}

static void testFindFilesWithPrefix(void) {
   using namespace CouchbaseDirectoryUtilities;

   std::vector<std::string> vec = findFilesWithPrefix("fs");
   expect(1, vec);
   contains("." PATH_SEPARATOR "fs", vec);

   vec = findFilesWithPrefix("fs", "d");
   expect(3, vec);
   contains("fs" PATH_SEPARATOR "d1", vec);
   contains("fs" PATH_SEPARATOR "d2", vec);
   contains("fs" PATH_SEPARATOR "d3", vec);
   vec = findFilesWithPrefix("fs", "1");
   expect(1, vec);
   contains("fs" PATH_SEPARATOR "1", vec);

   vec = findFilesWithPrefix("fs", "");
   expect(vfs.size() - 1, vec);

}

static void testFindFilesContaining(void) {
   using namespace CouchbaseDirectoryUtilities;
   std::vector<std::string> vec = findFilesContaining("fs", "");
   expect(vfs.size() - 1, vec);

   vec = findFilesContaining("fs", "2");
   expect(7, vec);
   contains("fs" PATH_SEPARATOR "d2", vec);
   contains("fs" PATH_SEPARATOR "e2", vec);
   contains("fs" PATH_SEPARATOR "f2c", vec);
   contains("fs" PATH_SEPARATOR "g2", vec);
   contains("fs" PATH_SEPARATOR "2", vec);
   contains("fs" PATH_SEPARATOR "2c", vec);
   contains("fs" PATH_SEPARATOR "2d", vec);
}

static void testRemove(void) {
   fclose(fopen("test-file", "w"));
   if (!CouchbaseDirectoryUtilities::rmrf("test-file")) {
      std::cerr << "expected to delete existing file" << std::endl;
   }
   if (CouchbaseDirectoryUtilities::rmrf("test-file")) {
      std::cerr << "Didn't expected to delete non-existing file" << std::endl;
   }

   if (!CouchbaseDirectoryUtilities::rmrf("fs")) {
      std::cerr << "Expected to nuke the entire fs directory recursively" << std::endl;
   }
}

int main(int argc, char **argv)
{
   testDirname();
   testBasename();


   vfs.push_back("fs");
   vfs.push_back("fs/d1");
   vfs.push_back("fs/d2");
   vfs.push_back("fs/e2");
   vfs.push_back("fs/f2c");
   vfs.push_back("fs/g2");
   vfs.push_back("fs/d3");
   vfs.push_back("fs/1");
   vfs.push_back("fs/2");
   vfs.push_back("fs/2c");
   vfs.push_back("fs/2d");
   vfs.push_back("fs/3");

   std::list<std::string>::iterator ii;
   for (ii = vfs.begin(); ii != vfs.end(); ++ii) {
      if (!CreateDirectory(*ii)) {
         if (!exists(*ii)) {
            std::cerr << "Fatal: failed to setup test directory: "
                      << *ii << std::endl;
            exit(EXIT_FAILURE);
         }
      }
   }

   testFindFilesWithPrefix();
   testFindFilesContaining();
   testRemove();

   return exit_value;
}
