#include "object.hpp"
#include <string>

namespace cisl {

inline std::shared_ptr<Character> make_char(char c) {
  return std::make_shared<Character>(Character(c));
}
inline std::vector<std::shared_ptr<Character>>
to_cisl_string(std::string chars) {
  std::vector<std::shared_ptr<Character>> s;
  for (auto c : chars) {
    s.push_back(make_char(c));
  }
  return s;
}

inline char raw(std::shared_ptr<Character> c) { return c->value; }
inline std::string to_std_string(String chars) {
  std::string s;
  for (auto o : chars.value) {
    s.push_back(raw(std::static_pointer_cast<Character>(o)));
  }
  return s;
}

ObjPtr map(const ObjPtr &obj, FuncObj func) {
  std::shared_ptr<Cons> c = std::static_pointer_cast<Cons>(obj);
  return std::make_shared<Cons>(Cons(func(c->car), map(c->cdr, func)));
}

ObjPtr map(const ObjPtr &ob1, const ObjPtr &ob2, FuncObj2 func) {
  std::shared_ptr<Cons> c1 = std::static_pointer_cast<Cons>(ob1);
  std::shared_ptr<Cons> c2 = std::static_pointer_cast<Cons>(ob2);
  return std::make_shared<Cons>(
      Cons(func(c1->car, c2->car), map(c1->cdr, c2->cdr, func)));
}
ObjPtr last(const ObjPtr &obj) {
  std::shared_ptr<Cons> c = std::static_pointer_cast<Cons>(obj);
  if (c->cdr == nullptr) {
    return c->car;
  }
  return last(c->cdr);
};

Type Integer::type() const { return INTEGER; }
String Integer::to_string() const {
  String s;
  std::string t = std::to_string(value);
  for (auto c : t) {
    s.value.push_back(make_char(c));
  }
  return s;
}

Type Float::type() const { return FLOAT; }
String Float::to_string() const {
  String s;
  std::string t = std::to_string(value);
  for (auto c : t) {
    s.value.push_back(make_char(c));
  }
  return s;
}

Character::Character(char value) : value(value) {}
Type Character::type() const { return CHARACTER; }
String Character::to_string() const {
  String s;
  s.value.push_back(make_char('#'));
  s.value.push_back(make_char('\\'));
  s.value.push_back(make_char(value));
  return s;
}

Array::Array() {}
Array::Array(std::vector<ObjPtr> value) {}
Type Array::type() const { return ARRAY; };
String Array::to_string() const {
  String s, t;
  for (auto v : value) {
    t = v->to_string();
    s.value.push_back(make_char('['));
    s.value.insert(s.value.end(), t.value.begin(), t.value.end());
    s.value.push_back(make_char(' '));
  }
  s.value.push_back(make_char(']'));
  return s;
}

String::String() : Array(){};

String::String(std::vector<std::shared_ptr<Character>> chars) {
  for (auto c : chars) {
    value.push_back(c);
  }
};

Type String::type() const { return STRING; }
String String::to_string() const {
  String s, t;
  for (auto v : value) {
    t = v->to_string();
    s.value.push_back(make_char('"'));
    s.value.insert(s.value.end(), t.value.begin(), t.value.end());
  }
  s.value.push_back(make_char('"'));
  return s;
}

Cons::Cons(const ObjPtr &car, const ObjPtr &cdr) : car(car), cdr(cdr){};
Type Cons::type() const { return CONS; }
String Cons::to_string() const {
  String s, t;
  t = car->to_string();
  s.value.push_back(make_char('('));
  s.value.insert(s.value.end(), t.value.begin(), t.value.end());
  t = String(to_cisl_string(" . "));
  s.value.insert(s.value.end(), t.value.begin(), t.value.end());
  t = cdr->to_string();
  s.value.insert(s.value.end(), t.value.begin(), t.value.end());
  s.value.push_back(make_char(')'));
  return s;
}

Type Nil::type() const { return NIL; }
String Nil::to_string() const { return to_cisl_string("nil"); }

Symbol::Symbol(String value) : value(value) {}
Type Symbol::type() const { return SYMBOL; }
String Symbol::to_string() const { return value; }

Macro::Macro(std::function<ObjPtr(const ObjPtr &)> macro) : macro(macro) {}
Macro::Macro(const ObjPtr &args, const ObjPtr &body, Table &vt, Table &ft) {
  macro = [&args, &body, &vt, &ft](const ObjPtr &vars) {
    Table local_vt = vt;
    Table local_ft = ft;
    map(args, vars, [&local_vt](const ObjPtr &a, const ObjPtr &v) {
      local_vt.at(to_std_string(a->to_string())) = v;
      return a;
    });
    return last(map(body, Eval(local_vt, local_ft)));
  };
}

Type Macro::type() const { return MACRO; }
String Macro::to_string() const { return String(to_cisl_string("#<MACRO>")); }
ObjPtr Macro::call(const ObjPtr &args) const { return macro(args); }

Function::Function(std::function<ObjPtr(const ObjPtr &)> function)
    : function(function) {}
Function::Function(const ObjPtr &args, const ObjPtr &body, Table &vt,
                   Table &ft) {
  function = [&args, &body, &vt, &ft](const ObjPtr &vars) {
    Table local_vt = vt;
    Table local_ft = ft;
    map(args, vars, [&local_vt](const ObjPtr &a, const ObjPtr &v) {
      local_vt.at(to_std_string(a->to_string())) = v;
      return a;
    });
    return last(map(body, Eval(local_vt, local_ft)));
  };
}

Type Function::type() const { return FUNCTION; }
String Function::to_string() const {
  return String(to_cisl_string("#<FUNCTION>"));
}
ObjPtr Function::call(const ObjPtr &args) const { return function(args); }

Eval::Eval(Table &vt, Table &ft) : vt(vt), ft(ft) {}
ObjPtr Eval::operator()(const ObjPtr &exp) {
  if (exp->type() == CONS) {
    Eval eval(vt, ft);
    auto cell = std::static_pointer_cast<Cons>(exp);
    auto obj = ft.at(to_std_string(cell->car->to_string()));
    if (obj->type() == FUNCTION) {
      auto func = std::static_pointer_cast<Function>(obj);
      return func->call(map(cell->cdr, eval));
    } else if (obj->type() == MACRO) {
      auto mac = std::static_pointer_cast<Macro>(obj);
      return eval(mac->call(cell->cdr));
    }
  } else if (exp->type() == SYMBOL) {
    return vt.at(to_std_string(exp->to_string()));
  }
  return exp;
}
}
