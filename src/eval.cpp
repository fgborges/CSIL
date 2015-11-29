#include "lisp.hpp"
#include "util.hpp"
#include <unordered_map>

namespace islisp {
namespace core {

extern std::unordered_map<Symbol, ObjPtr> buildin_macros;
extern std::unordered_map<Symbol, ObjPtr> buildin_functions;

ObjPtr eval(ObjPtr obj, Env &env) {
  if (obj->type == Object::ATOM) {
    if (obj->atom.type == Atom::SYMBOL) {
      return env.variables.at(obj->atom.symbol);
    } else {
      return obj;
    }
  } else {
    auto sym = car(obj)->atom.symbol;
    if (env.functions.count(sym) != 0) {
      for (auto a = cdr(obj); a != nullptr; a = cdr(a)) {
        a->cons.car = eval(car(a), env);
      }
      return env.functions.at(sym)->atom.function(cdr(obj));
    } else if (env.macros.count(sym) != 0) {
      return eval(env.macros.at(sym)->atom.function(cdr(obj)), env);
    } else if (buildin_functions.count(sym) != 0) {
      for (auto a = cdr(obj); a != nullptr; a = cdr(a)) {
        a->cons.car = eval(car(a), env);
      }
      return buildin_functions.at(sym)->atom.function(cdr(obj));
    } else if (buildin_macros.count(sym) != 0) {
      return eval(buildin_macros.at(sym)->atom.function(cdr(obj)), env);
    } else if (sym == "quote") {
      return car(cdr(obj));
    } else if (sym == "if") {
      auto test = cadr(obj);
      auto then = caddr(obj);
      auto otherwise = cadddr(obj);
      if (eval(test, env) != nullptr) {
        return eval(then, env);
      }
      return eval(otherwise, env);
    } else if (sym == "apply") {
      return cadr(obj)->atom.function(caddr(obj));
    } else if (sym == "lambda") {
      auto args = cadr(obj);
      auto body = caddr(obj);
      auto lambda = std::make_shared<Object>();
      createAtom(lambda, [=](ObjPtr vars) -> ObjPtr {
        ObjPtr result;
        auto closure = env;
        for (auto a = args, v = vars; v != nullptr && a != nullptr;
             a = cdr(a), v = cdr(v)) {
          if (car(a)->atom.symbol == "&rest") {
            closure.variables[cadr(a)->atom.symbol] = v;
            break;
          }
          closure.variables[car(a)->atom.symbol] = car(v);
        }
        for (auto b = body; b != nullptr; b = cdr(b)) {
          result = eval(car(b), closure);
        }
        return result;
      });
      return lambda;
    } else if (sym == "define") {
      auto name = cadr(obj);
      auto value = eval(caddr(obj), env);
      if (value->type == Object::ATOM && value->atom.type == Atom::FUNCTION) {
        env.functions[name->atom.symbol] = value;
      } else {
        env.variables[name->atom.symbol] = value;
      }
      return name;
    }
  }
  return nullptr;
}
}
}
