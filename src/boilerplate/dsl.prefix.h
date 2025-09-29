#include "../src/ir.h"

#include "../current/typesystem/serialization/json.h"

struct Ctx final {
  MaroonIRScenarios out;

  std::string current_maroon_name;
  std::string current_fiber_name;
  std::string current_function_name;

  uint32_t next_placeholder_idx = 0;
  std::vector<std::pair<uint32_t, size_t>> blocks_stack;  // Where to insert the newly completed blocks.

  std::vector<std::unique_ptr<MaroonIRBlock>> current_fn_blocks_stack;

  bool InFunction() const { return !current_fn_blocks_stack.empty(); }

  void EnterFunction(MaroonIRFunction& fn, uint32_t line) {
    if (!current_fn_blocks_stack.empty()) {
      std::cerr << "NE" << std::endl;
      std::exit(1);
    }
    current_fn_blocks_stack.push_back(std::unique_ptr<MaroonIRBlock>(new MaroonIRBlock()));
    current_fn_blocks_stack.back()->line = line;
  }

  void LeaveFunction() {
    if (current_fn_blocks_stack.size() != 1) {
      std::cerr << "Internal invariant failed: Should be at exactly one block depth at function end." << std::endl;
    }
    out.maroon[current_maroon_name].fibers[current_fiber_name].functions[current_function_name].body =
        std::move(*current_fn_blocks_stack.back());
    current_fn_blocks_stack.clear();
    current_function_name = "";
  }

  uint32_t EnterBlock(uint32_t line) {
    MaroonIRBlockPlaceholder _p;
    auto const key = _p._idx = ++next_placeholder_idx;
    _p.line = line;
    size_t const saved_index = AddToBlock(std::move(_p));
    blocks_stack.push_back({key, saved_index});
    current_fn_blocks_stack.push_back(std::unique_ptr<MaroonIRBlock>(new MaroonIRBlock()));
    return key;
  }

  size_t AddToBlock(MaroonIRStmtOrBlock c) {
    size_t i = current_fn_blocks_stack.back()->code.size();
    current_fn_blocks_stack.back()->code.push_back(std::move(c));
    return i;
  }

  MaroonIRStmtOrBlock ExtractLastStmt() {
    MaroonIRStmtOrBlock res = std::move(current_fn_blocks_stack.back()->code.back());
    current_fn_blocks_stack.back()->code.pop_back();
    return res;
  }

  size_t BlocksDepth() const { return current_fn_blocks_stack.size(); }

  void AddVarToBlock(MaroonIRVar var) { current_fn_blocks_stack.back()->vars.push_back(std::move(var)); }

  void MarkInnerBlockAsCompleted(size_t user_key) {
    if (blocks_stack.empty()) {
      std::cerr << "WTF0!" << std::endl;
      std::exit(1);
    }
    auto [key, idx] = blocks_stack.back();
    blocks_stack.pop_back();
    if (key != user_key) {
      std::cerr << "WTF1!" << std::endl;
      std::exit(1);
    }
    std::unique_ptr<MaroonIRBlock> blk = std::move(current_fn_blocks_stack.back());
    current_fn_blocks_stack.pop_back();
    if (current_fn_blocks_stack.back()->code.empty()) {
      std::cerr << "WTF2!" << std::endl;
      std::exit(1);
    }
    if (idx >= current_fn_blocks_stack.back()->code.size()) {
      std::cerr << "WTF3" << std::endl;
      std::exit(1);
    }
    if (!Exists<MaroonIRBlockPlaceholder>(current_fn_blocks_stack.back()->code[idx])) {
      std::cerr << "WTF4!" << std::endl;
      std::exit(1);
    }
    if (Value<MaroonIRBlockPlaceholder>(current_fn_blocks_stack.back()->code[idx])._idx != key) {
      std::cerr << "WTF5!" << std::endl;
      std::exit(1);
    }
    blk->line = Value<MaroonIRBlockPlaceholder>(current_fn_blocks_stack.back()->code[idx]).line;

    current_fn_blocks_stack.back()->code[idx] = std::move(*blk);
  }
};

struct RegisterMaroon final {
  Ctx& ctx;
  bool entered = false;

  RegisterMaroon(Ctx& ctx, std::string const& name, uint32_t line) : ctx(ctx) {
    if (ctx.out.maroon.count(name)) {
      std::cerr << "`MAROON(" << name << ")` is defined more than once." << std::endl;
      std::exit(1);
    }
    ctx.current_maroon_name = name;
    ctx.out.maroon[ctx.current_maroon_name].line = line;
  }

  ~RegisterMaroon() {
    if (!entered) {
      std::cerr << "FOO1" << std::endl;
      std::exit(1);
    }
  }

  void operator<<(std::function<void()> f) {
    if (ctx.current_maroon_name.empty()) {
      std::cerr << "FOO2" << std::endl;
      std::exit(1);
    }
    if (entered) {
      std::cerr << "FOO3" << std::endl;
      std::exit(1);
    }
    entered = true;
    f();
    ctx.current_maroon_name = "";
  }
};

struct RegisterFiber final {
  Ctx& ctx;
  bool entered = false;

  RegisterFiber(Ctx& ctx, std::string const& name, uint32_t line) : ctx(ctx) {
    if (ctx.current_maroon_name.empty()) {
      std::cerr << "`FIBER(" << name << ")` should be defined within some `MAROON()`." << std::endl;
      std::exit(1);
    }
    if (ctx.out.maroon[ctx.current_maroon_name].fibers.count(name)) {
      std::cerr << "`FIBER(" << name << ")` is defined more than once in `MAROON(" << ctx.current_maroon_name << ")`."
                << std::endl;
      std::exit(1);
    }
    ctx.current_fiber_name = name;
    ctx.out.maroon[ctx.current_maroon_name].fibers[ctx.current_fiber_name].line = line;
  }

  ~RegisterFiber() {
    if (!entered) {
      std::cerr << "BAR1" << std::endl;
      std::exit(1);
    }
  }

  void operator<<(std::function<void()> f) {
    if (ctx.current_fiber_name.empty()) {
      std::cerr << "BAR2" << std::endl;
      std::exit(1);
    }
    if (entered) {
      std::cerr << "BAR3" << std::endl;
      std::exit(1);
    }
    entered = true;
    f();
    ctx.current_fiber_name = "";
  }
};

struct RegisterFn final {
  Ctx& ctx;
  bool entered = false;

  RegisterFn(Ctx& ctx, std::string const& name, uint32_t line) : ctx(ctx) {
    if (ctx.current_fiber_name.empty()) {
      std::cerr << "`FN(" << name << ")` should be defined within some `FIBER()`." << std::endl;
      std::exit(1);
    }
    if (ctx.out.maroon[ctx.current_maroon_name].fibers[ctx.current_fiber_name].functions.count(name)) {
      std::cerr << "`FN(" << name << ")` is defined more than once in `FIBER(" << ctx.current_fiber_name
                << ")` of `MAROON(" << ctx.current_maroon_name << ")`." << std::endl;
      std::exit(1);
    }
    MaroonIRFunction& fn = ctx.out.maroon[ctx.current_maroon_name].fibers[ctx.current_fiber_name].functions[name];
    ctx.current_function_name = name;
    fn.line = line;
    ctx.EnterFunction(fn, line);
  }

  ~RegisterFn() {
    if (!entered) {
      std::cerr << "MEH1" << std::endl;
      std::exit(1);
    }
  }

  void operator<<(std::function<void()> f) {
    if (entered) {
      std::cerr << "MEH2" << std::endl;
      std::exit(1);
    }
    entered = true;
    f();
    ctx.LeaveFunction();
  }
};

struct RegisterStmt final {
  Ctx& ctx;

  RegisterStmt(Ctx& ctx, uint32_t line, std::string const& stmt) : ctx(ctx) {
    if (!ctx.InFunction()) {
      std::cerr << "`STMT()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    MaroonIRStmt obj;
    obj.line = line;
    obj.stmt = stmt;

    ctx.AddToBlock(std::move(obj));
  }
};

struct RegisterIf final {
  Ctx& ctx;

  RegisterIf(Ctx& ctx, std::string condition, std::function<void()> yes, std::function<void()> no, uint32_t line)
      : ctx(ctx) {
    if (!ctx.InFunction()) {
      std::cerr << "`IF()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    // NOTE(dkorolev): Trivially construct two blocks and then extract them.
    yes();
    no();
    MaroonIRIf cond;
    cond.line = line;
    cond.cond = condition;
    cond.no = ctx.ExtractLastStmt();
    cond.yes = ctx.ExtractLastStmt();
    ctx.AddToBlock(std::move(cond));
  }
};

struct RegisterBlock final {
  Ctx& ctx;
  uint32_t block_key;
  size_t save_stack_depth;

  bool entered = false;

  RegisterBlock(Ctx& ctx, uint32_t line) : ctx(ctx) {
    if (!ctx.InFunction()) {
      std::cerr << "`BLOCK()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    block_key = ctx.EnterBlock(line);
    save_stack_depth = ctx.BlocksDepth();
  }

  ~RegisterBlock() {
    if (!entered) {
      std::cout << "FFUUUUU1\n";
      std::exit(1);
    }
  }

  void operator<<(std::function<void()> f) {
    if (entered) {
      std::cout << "FFUUUUU2\n";
      std::exit(1);
    }
    entered = true;
    f();
    if (ctx.BlocksDepth() != save_stack_depth) {
      std::cerr << "Internal error, stack depth mismatch on closing the block." << std::endl;
      std::exit(1);
    }
    ctx.MarkInnerBlockAsCompleted(block_key);
  }
};

enum class VarTypes {
  U64,
};

inline void RegisterVar(
    Ctx& ctx, std::string const& name, VarTypes type, std::string const& init_as_string, uint32_t line) {
  if (!ctx.InFunction()) {
    std::cerr << "`VAR()` is only legal inside an `FN()`." << std::endl;
    std::exit(1);
  }

  MaroonIRVar var;
  var.line = line;
  var.name = name;
  var.type = "TODO(dkorolev): Implement this.";
  var.init = init_as_string;

  ctx.AddVarToBlock(std::move(var));
}

int main() {
  Ctx ctx;
