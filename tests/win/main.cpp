#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <kodak/win/ComApartment.h>

int main(int argc, char** argv) {
    const kodak::win::ComApartment com; // WIC in the PNG tests
    return doctest::Context(argc, argv).run();
}
