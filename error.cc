struct Pos
{
    int line;
    std::string filename;
};

struct PosError
{
    PosError(Pos pos, std::string message) : pos{pos}, message{std::move(message)} {}

    Pos getPos() const { return pos; }
    const std::string& getMessage() const { return message; }

private:
    Pos pos;
    std::string message;
};

