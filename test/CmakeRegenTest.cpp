#include "test.h"

#include "CmakeRegen.h"

TEST(MakeCmakeComment_Oneline) {
  const std::string contents("A oneline comment");
  const std::string expectedComment("# A oneline comment\n");

  std::string comment;
  MakeCmakeComment(comment, contents);
  ASSERT(comment == expectedComment);
}

TEST(MakeCmakeComment_Multiline) {
  const std::string contents("Multiple\n"
                             "lines\n"
                             "in\n"
                             "comment");
  const std::string expectedComment("# Multiple\n"
                                    "# lines\n"
                                    "# in\n"
                                    "# comment\n");

  std::string comment;
  MakeCmakeComment(comment, contents);
  ASSERT(comment == expectedComment);
}

TEST(MakeCmakeComment_Multiline_LastWithEndLine) {
  const std::string contents("Multiple\n"
                             "lines\n"
                             "in\n"
                             "comment\n");
  const std::string expectedComment("# Multiple\n"
                                    "# lines\n"
                                    "# in\n"
                                    "# comment\n");

  std::string comment;
  MakeCmakeComment(comment, contents);
  ASSERT(comment == expectedComment);
}
