#include <iostream>
#include <platform/dirutils.h>
#include <cstdlib>
#include <limits>
#include <list>
#include <string>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#ifdef WIN32
#define NOMINMAX
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

static void expect(const bool exp, const bool val) {
    if (exp != val) {
        std::cerr << "Expected " << exp << " got [" << val << "]" << std::endl;
        exit_value = EXIT_FAILURE;
    }
}

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
   expect("foo", cb::io::dirname("foo\\bar"));
   expect("foo", cb::io::dirname("foo/bar"));

   // Make sure that we remove double an empty chunk
   expect("foo", cb::io::dirname("foo\\\\bar"));
   expect("foo", cb::io::dirname("foo//bar"));

   // Make sure that we handle the case without a directory
   expect(".", cb::io::dirname("bar"));
   expect(".", cb::io::dirname(""));

   // Absolute directories
   expect("\\", cb::io::dirname("\\bar"));
   expect("\\", cb::io::dirname("\\\\bar"));
   expect("/", cb::io::dirname("/bar"));
   expect("/", cb::io::dirname("//bar"));

   // Test that we work with multiple directories
   expect("1/2/3/4/5", cb::io::dirname("1/2/3/4/5/6"));
   expect("1\\2\\3\\4\\5", cb::io::dirname("1\\2\\3\\4\\5\\6"));
   expect("1/2\\4/5", cb::io::dirname("1/2\\4/5\\6"));
}

static void testBasename(void) {
   expect("bar", cb::io::basename("foo\\bar"));
   expect("bar", cb::io::basename("foo/bar"));
   expect("bar", cb::io::basename("foo\\\\bar"));
   expect("bar", cb::io::basename("foo//bar"));
   expect("bar", cb::io::basename("bar"));
   expect("", cb::io::basename(""));
   expect("bar", cb::io::basename("\\bar"));
   expect("bar", cb::io::basename("\\\\bar"));
   expect("bar", cb::io::basename("/bar"));
   expect("bar", cb::io::basename("//bar"));
   expect("6", cb::io::basename("1/2/3/4/5/6"));
   expect("6", cb::io::basename("1\\2\\3\\4\\5\\6"));
   expect("6", cb::io::basename("1/2\\4/5\\6"));
}

static void testFindFilesWithPrefix(void) {
   using namespace cb::io;

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
   using namespace cb::io;
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
   if (!cb::io::rmrf("test-file")) {
      std::cerr << "expected to delete existing file" << std::endl;
   }
   if (cb::io::rmrf("test-file")) {
      std::cerr << "Didn't expected to delete non-existing file" << std::endl;
   }

   if (!cb::io::rmrf("fs")) {
      std::cerr << "Expected to nuke the entire fs directory recursively" << std::endl;
   }
}

static void testIsDirectory(void) {
    using namespace cb::io;
#ifdef WIN32
    expect(true, isDirectory("c:\\"));
#else
    expect(true, isDirectory("/"));
#endif
    expect(true, isDirectory("."));
    expect(false, isDirectory("/it/would/suck/if/this/exists"));
    FILE *fp = fopen("isDirectoryTest", "w");
    if (fp == NULL) {
        std::cerr << "Failed to create test file" << std::endl;
        exit_value = EXIT_FAILURE;
    } else {
        using namespace std;
        fclose(fp);
        expect(false, isDirectory("isDirectoryTest"));
        remove("isDirectoryTest");
    }
}

static void testIsFile(void) {
   using namespace cb::io;
   expect(false, isFile("."));
   FILE* fp = fopen("plainfile", "w");
   if (fp == nullptr) {
      std::cerr << "Failed to create test file" << std::endl;
      exit_value = EXIT_FAILURE;
   } else {
      fclose(fp);
      expect(true, isFile("plainfile"));
      rmrf("plainfile");
   }
}

static void testMkdirp(void) {
    using namespace cb::io;

#ifndef WIN32
    expect(false, mkdirp("/it/would/suck/if/I/could/create/this"));
#endif
    expect(true, mkdirp("."));
    expect(true, mkdirp("/"));
    expect(true, mkdirp("foo/bar"));
    expect(true, isDirectory("foo/bar"));
    rmrf("foo");
}

static void testGetCurrentDirectory() {
   try {
      auto cwd = cb::io::getcwd();
      // I can't really determine the correct value here, but it shouldn't be
      // empty ;-)
      if (cwd.empty()) {
         std::cerr << "FAIL: cwd should not be an empty string" << std::endl;
         exit(EXIT_FAILURE);
      }
   } catch (const std::exception& ex) {
      std::cerr << "FAIL: " << ex.what() << std::endl;
      exit(EXIT_FAILURE);
   }
}

static void testCbMktemp() {
   auto filename = cb::io::mktemp("foo");
   if (filename.empty()) {
      std::cerr << "FAIL: Expected to create tempfile without mask"
                << std::endl;
      exit(EXIT_FAILURE);
   }

   if (!cb::io::isFile(filename)) {
      std::cerr << "FAIL: Expected mktemp to create file" << std::endl;
      exit(EXIT_FAILURE);
   }

   if (!cb::io::rmrf(filename)) {
      std::cerr << "FAIL: failed to remove temporary file" << std::endl;
      exit(EXIT_FAILURE);
   }

   filename = cb::io::mktemp("barXXXXXX");
   if (filename.empty()) {
      std::cerr << "FAIL: Expected to create tempfile with mask"
                << std::endl;
      exit(EXIT_FAILURE);
   }

   if (!cb::io::isFile(filename)) {
      std::cerr << "FAIL: Expected mktemp to create file" << std::endl;
      exit(EXIT_FAILURE);
   }

   if (!cb::io::rmrf(filename)) {
      std::cerr << "FAIL: failed to remove temporary file" << std::endl;
      exit(EXIT_FAILURE);
   }
}

void testMaximizeFileDescriptors() {
   auto limit = cb::io::maximizeFileDescriptors(32);
   if (limit <= 32) {
      std::cerr << "FAIL: I should be able to set it to at least 32"
                << std::endl;
      exit(EXIT_FAILURE);
   }

   limit = cb::io::maximizeFileDescriptors(std::numeric_limits<uint32_t>::max());
   if (limit != std::numeric_limits<uint32_t>::max()) {
      // windows don't have a max limit, and that could be for other platforms
      // as well..
      if (cb::io::maximizeFileDescriptors(limit + 1) != limit) {
         std::cerr << "FAIL: I expected maximizeFileDescriptors to return "
                   << "the same max limit" << std::endl;
         exit(EXIT_FAILURE);
      }
   }

   limit = cb::io::maximizeFileDescriptors(std::numeric_limits<uint64_t>::max());
   if (limit != std::numeric_limits<uint64_t>::max()) {
      // windows don't have a max limit, and that could be for other platforms
      // as well..
      if (cb::io::maximizeFileDescriptors(limit + 1) != limit) {
         std::cerr << "FAIL: I expected maximizeFileDescriptors to return "
                   << "the same max limit" << std::endl;
         exit(EXIT_FAILURE);
      }
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

   testIsDirectory();
   testIsFile();
   testMkdirp();
   testGetCurrentDirectory();
   testCbMktemp();

   testMaximizeFileDescriptors();

   return exit_value;
}
