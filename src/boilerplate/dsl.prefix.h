#include "../src/ir.h"

#include "../current/typesystem/serialization/json.h"

struct Ctx final {
  MaroonIRScenarios out;

  std::string current_maroon_name;
  std::string current_fiber_name;
  std::string current_function_name;
  MaroonIRNamespace* current_maroon_ptr = nullptr;
  MaroonIRFiber* current_fiber_ptr = nullptr;
  std::vector<MaroonIRStatement*> current_function_stack_ptrs;
};

struct RegisterMaroon final {
  Ctx& ctx;

  RegisterMaroon(Ctx& ctx, std::string const& name) : ctx(ctx) {
    if (ctx.out.maroon.count(name)) {
      std::cerr << "`MAROON(" << name << ")` is defined more than once." << std::endl;
      std::exit(1);
    }
    ctx.current_maroon_ptr = &ctx.out.maroon[name];
    ctx.current_maroon_name = name;
  }

  ~RegisterMaroon() {
    ctx.current_maroon_ptr = nullptr;
    ctx.current_maroon_name = "";
  }

  void operator<<(std::function<void()> f) { f(); }
};

struct RegisterFiber final {
  Ctx& ctx;

  RegisterFiber(Ctx& ctx, std::string const& name) : ctx(ctx) {
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
  }

  ~RegisterFiber() {
    ctx.current_fiber_ptr = nullptr;
    ctx.current_fiber_name = "";
  }

  void operator<<(std::function<void()> f) { f(); }
};

struct RegisterFn final {
  Ctx& ctx;

  RegisterFn(Ctx& ctx, std::string const& name) : ctx(ctx) {
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
    fn = MaroonIRNop();  // TODO(dkorolev): Figure out how to test for legality of empty functions.
    ctx.current_function_name = name;
    ctx.current_function_stack_ptrs.clear();
    ctx.current_function_stack_ptrs.push_back(&fn);
  }

  ~RegisterFn() {
    ctx.current_function_name = "";
    ctx.current_function_stack_ptrs.clear();
  }

  void operator<<(std::function<void()> f) { f(); }
};

struct RegisterStmt final {
  Ctx& ctx;

  RegisterStmt(Ctx& ctx, std::string const& stmt) : ctx(ctx) {
    if (ctx.current_function_stack_ptrs.empty()) {
      std::cerr << "`STMT()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    MaroonIRCode src;
    src.code = stmt;

    // First, we need to handle whether the "current" placeholder is a block.
    MaroonIRStatement& dst = ([&ctx]() -> MaroonIRStatement& {
      MaroonIRStatement& tail = *ctx.current_function_stack_ptrs.back();
      if (Exists<MaroonIRBlock>(tail)) {
        return Value<MaroonIRBlock>(tail).stmt;
      } else {
        return tail;
      }
    })();

    // If `Nop` replace by code, if `Seq` add to it, otherwise (code or block) turn into a `Seq`.
    if (Exists<MaroonIRNop>(dst)) {
      dst = std::move(src);
    } else if (Exists<MaroonIRSeq>(dst)) {
      Value<MaroonIRSeq>(dst).seq.push_back(std::move(src));
    } else {
      MaroonIRSeq out;
      out.seq.push_back(std::move(dst));
      out.seq.push_back(std::move(src));
      dst = std::move(out);
    }
  }
};

struct RegisterBlock final {
  Ctx& ctx;
  size_t save_stack_depth;

  RegisterBlock(Ctx& ctx) : ctx(ctx) {
    if (ctx.current_function_stack_ptrs.empty()) {
      std::cerr << "`BLOCK()` is only legal inside an `FN()`." << std::endl;
      std::exit(1);
    }

    MaroonIRStatement& dst = *ctx.current_function_stack_ptrs.back();

    // What should be populated is `Seq`, which, upon destruction of this `Block` will get appended a `Nop`.
    MaroonIRBlock blk;
    blk.stmt = MaroonIRNop();  // TODO(dkorolev): Might not need this with a default constructor.
    MaroonIRSeq src;
    src.seq.push_back(std::move(blk));

    // If `Nop` replace by code, if `Seq` add to it, otherwise (code or block) turn into a `Seq`.
    if (Exists<MaroonIRNop>(dst)) {
      dst = std::move(src);
    } else if (Exists<MaroonIRSeq>(dst)) {
      Value<MaroonIRSeq>(dst).seq.push_back(std::move(src));
    } else {
      MaroonIRSeq out;
      out.seq.push_back(std::move(dst));
      out.seq.push_back(std::move(src));
      dst = std::move(out);
    }

    ctx.current_function_stack_ptrs.push_back(&Value<MaroonIRSeq>(dst).seq.back());
    save_stack_depth = ctx.current_function_stack_ptrs.size();
  }

  ~RegisterBlock() {
    if (ctx.current_function_stack_ptrs.size() != save_stack_depth) {
      std::cerr << "Internal error, stack depth mismatch on closing the block." << std::endl;
      std::exit(1);
    }
    ctx.current_function_stack_ptrs.pop_back();
  }
};

enum class VarTypes {
  U64,
};

inline void RegisterVar(Ctx& ctx, std::string const& name, VarTypes type, std::string const& init_as_string) {
  if (ctx.current_function_stack_ptrs.empty()) {
    std::cerr << "`VAR()` is only legal inside an `FN()`." << std::endl;
    std::exit(1);
  }

  MaroonIRVar var;
  var.name = name;
  var.type = "TODO(dkorolev): Implement this.";
  var.init = init_as_string;

  MaroonIRStatement& dst = *ctx.current_function_stack_ptrs.back();
  if (!Exists<MaroonIRBlock>(dst)) {
    MaroonIRBlock new_blk;
    new_blk.stmt = std::move(dst);
    dst = std::move(new_blk);
  }
  Value<MaroonIRBlock>(dst).vars.push_back(std::move(var));
}

int main() {
  Ctx ctx;
