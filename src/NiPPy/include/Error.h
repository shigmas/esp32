#ifndef BLAB_ERROR
#define BLAB_ERROR

#include <optional>
#include <string>

class _Error {
public:
    _Error(const std::string& msg = ""): _message(msg) {}

    const std::string& String() { return _message; }

  private:
    std::string _message;
};

#define BlabError std::optional<_Error>

#endif // BLAB_ERROR
