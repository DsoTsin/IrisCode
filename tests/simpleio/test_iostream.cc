#include <fstream>
#include <iostream>

using namespace std;

int main() {
  ofstream of("test.txt");
  of << "test";
  of.close();

  string content;
  ifstream inf("test.txt");
  inf >> content;
  inf.close();

  cout << content << endl;
  return 0;
}