#ifndef RAII_H
#define RAII_H

template<class T> class RAII {
  public:
    RAII(const T &t): t{t} { }
    ~RAII() { t(); }

  private:
    T t;
};

template<class T> RAII<T> make_raii(const T &t) {
  return RAII<T>(t);
}

#endif
