#include "../src/ir.h"

#include "../current/typesystem/serialization/json.h"

struct Ctx final {
  MaroonIRScenarios out;

  std::string current_maroon_name;
  std::string current_fiber_name;
  std::string current_function_name;
  MaroonIRNamespace* current_maroon_ptr = nullptr;
  MaroonIRFiber* current_fiber_ptr = nullptr;
  std::vector<MaroonIRBlock*> current_fn_blocks_stack;
};

struct RegisterMaroon final {
  Ctx& ctx;

  RegisterMaroon(Ctx& ctx, std::string const& name, uint32_t line) : ctx(ctx) {
    if (ctx.out.maroon.count(name)) {
      std::cerr << "`MAROON(" << name << ")` is defined more than once." << std::endl;
      std::exit(1);
    }
    ctx.current_maroon_ptr = &ctx.out.maroon[name];
    ctx.current_maroon_name = name;
    ctx.current_maroon_ptr->line = line;
  }

  ~RegisterMaroon() {
    ctx.current_maroon_ptr = nullptr;
    ctx.current_maroon_name = "";
  }

  void operator<<(std::function<void()> f) { f(); }
};

struct RegisterFiber final {
  Ctx& ctx;

  RegisterFiber(Ctx& ctx, std::string const& name, uint32_t line) : ctx(ctx) {
    if (!ctx.current_maroon_ptr) {
      std::cerr << "`FIBER(" << name << ")` should be defined within some `MAROON()`." << std::endl;
      std::exit(1);
    }
    if (ctx.current_maroon_ptr->fibers.count(name)) {
      std::cerr << "`FIBER(" << name << ")` is defined more than once in `MAROON(" << ctx.current_maroon_name << ")`."
                << std::endl;
      std::exit(1);
    }
    ctx.current_fiber_ptr = &ctx.current_maroon_ptr->fibers[name];
    ctx.current_fiber_name = name;
    ctx.current_fiber_ptr->line = line;
  }

  ~RegisterFiber() {
    ctx.current_fiber_ptr = nullptr;
    ctx.current_fiber_name = "";
  }

  void operator<<(std::function<void()> f) { f(); }
};

struct RegisterFn final {
  Ctx& ctx;

  RegisterFn(Ctx& ctx, std::string const& name, uint32_t line) : ctx(ctx) {
    if (ctx.current_fiber_name.empty()) {
      std::cerr << "`FN(" << name << ")` should be defined within some `FIBER()`." << std::endl;
      std::exit(1);
    }
    if (ctx.current_fiber_ptr->functions.count(name)) {
      std::cerr << "`FN(" << name << ")` is defined more than once in `FIBER(" << ctx.current_fiber_name
                << ")` of `MAROON(" << ctx.current_maroon_name << ")`." << std::endl;
      std::exit(1);
    }
    MaroonIRFunction& fn = ctx.current_fiber_ptr->functions[name];
    ctx.current_function_name = name;
    fn.line = line;
    fn.body.line = line;
    ctx.current_fn_blocks_stack.clear();
    ctx.current_fn_blocks_stack.push_back(&fn.body);
  }

  ~RegisterFn() {
    ctx.current_function_name = "";
    if (ctx.current_fn_blocks_stack.size() != 1) {
      std::cerr << "Internal invariant failed: Should be at exactly one block depth at function end." << std::endl;
    }
    ctx.current_fn_blocks_stack.clear();
  }

  void operator<<(std::function<void()> f) { f(); }
};

struct RegisterStmt final {
  Ctx& ctx;

  RegisterStmt(Ctx& ctx, uint32_t line, std::string const& stmt) : ctx(ctx) {
    if (ctx.current_fn_blocks_stack.empty()) {
      std::cerr << "`STMT()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    MaroonIRStmt obj;
    obj.line = line;
    obj.stmt = stmt;

    ctx.current_fn_blocks_stack.back()->code.push_back(std::move(obj));
  }
};

struct RegisterIf final {
  Ctx& ctx;

  RegisterIf(Ctx& ctx, std::string condition, std::function<void()> yes, std::function<void()> no, uint32_t line)
      : ctx(ctx) {
    if (ctx.current_fn_blocks_stack.empty()) {
      std::cerr << "`IF()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    // NOTE(dkorolev): Trivially construct two blocks and then extract them.
    yes();
    no();
    MaroonIRIf cond;
    cond.line = line;
    cond.cond = condition;
    cond.no = std::move(ctx.current_fn_blocks_stack.back()->code.back());
    ctx.current_fn_blocks_stack.back()->code.pop_back();
    cond.yes = std::move(ctx.current_fn_blocks_stack.back()->code.back());
    ctx.current_fn_blocks_stack.back()->code.pop_back();
    ctx.current_fn_blocks_stack.back()->code.push_back(std::move(cond));
  }
};

struct RegisterBlock final {
  Ctx& ctx;
  size_t save_stack_depth;

  RegisterBlock(Ctx& ctx, uint32_t line) : ctx(ctx) {
    if (ctx.current_fn_blocks_stack.empty()) {
      std::cerr << "`BLOCK()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    MaroonIRBlock blk;
    blk.line = line;
    ctx.current_fn_blocks_stack.back()->code.push_back(std::move(blk));
    ctx.current_fn_blocks_stack.push_back(&Value<MaroonIRBlock>(ctx.current_fn_blocks_stack.back()->code.back()));
    save_stack_depth = ctx.current_fn_blocks_stack.size();
  }

  ~RegisterBlock() {
    if (ctx.current_fn_blocks_stack.size() != save_stack_depth) {
      std::cerr << "Internal error, stack depth mismatch on closing the block." << std::endl;
      std::exit(1);
    }
    ctx.current_fn_blocks_stack.pop_back();
  }

  void operator<<(std::function<void()> f) { f(); }
};

enum class VarTypes {
  U64,
};

inline void RegisterVar(
    Ctx& ctx, std::string const& name, VarTypes type, std::string const& init_as_string, uint32_t line) {
  if (ctx.current_fn_blocks_stack.empty()) {
    std::cerr << "`VAR()` is only legal inside an `FN()`." << std::endl;
    std::exit(1);
  }

  MaroonIRVar var;
  var.line = line;
  var.name = name;
  var.type = "TODO(dkorolev): Implement this.";
  var.init = init_as_string;

  ctx.current_fn_blocks_stack.back()->vars.push_back(std::move(var));
}

int main() {
  Ctx ctx;
