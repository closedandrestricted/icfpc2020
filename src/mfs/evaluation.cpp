#include "evaluation.h"

#include "dictionary.h"
#include "function_type.h"
#include "glyph_type.h"

#include "common/base.h"

#include <vector>

namespace {
Node* GetI(std::vector<Node*>& current_path, unsigned index) {
  return (index < current_path.size())
             ? current_path[current_path.size() - index - 1]
             : nullptr;
}

unsigned ExpectedParameters(FunctionType ftype) {
  switch (ftype) {
    case FunctionType::SUCCESSOR:
    case FunctionType::PREDECESSOR:
    case FunctionType::MODULATE:
    case FunctionType::DEMODULATE:
    case FunctionType::SEND:
    case FunctionType::NEGATE:
    case FunctionType::POWER_OF_TWO:
    case FunctionType::I_COMBINATOR:
    case FunctionType::CAR__FIRST:
    case FunctionType::CDR__TAIL:
    case FunctionType::NIL__EMPTY_LIST:
    case FunctionType::IS_NIL:
      // case FunctionType::LOG2:
      // case FunctionType::LENGTH:
      return 1;
    case FunctionType::SUM:
    case FunctionType::PRODUCT:
    case FunctionType::DIVISION:
    case FunctionType::EQUALITY:
    case FunctionType::STRICT_LESS:
    case FunctionType::K_COMBINATOR:
    case FunctionType::FALSE__SECOND:
      // case FunctionType::CONCAT:
      return 2;
    case FunctionType::S_COMBINATOR:
    case FunctionType::C_COMBINATOR:
    case FunctionType::B_COMBINATOR:
    case FunctionType::CONS__PAIR:
    case FunctionType::VECTOR:
    case FunctionType::IF0:
      return 3;
    default:
      return unsigned(-1);
  }
}

Node* ApplyFunction(Node* node, std::vector<Node*>& current_path) {
  assert(node && node->data.type == GlyphType::FUNCTION);
  assert(current_path.size() >= ExpectedParameters(node->data.ftype));
  Node *p0 = GetI(current_path, 0), *p1 = GetI(current_path, 1),
       *p2 = GetI(current_path, 2);
  switch (node->data.ftype) {
    case FunctionType::SUCCESSOR:
      Evaluate(p0->r);
      assert(p0->r->data.type == GlyphType::NUMBER);
      p0->data.type = GlyphType::NUMBER;
      p0->data.value = p0->r->data.value + 1;
      return p0;
    case FunctionType::PREDECESSOR:
      Evaluate(p0->r);
      assert(p0->r->data.type == GlyphType::NUMBER);
      p0->data.type = GlyphType::NUMBER;
      p0->data.value = p0->r->data.value - 1;
      return p0;
    case FunctionType::MODULATE:
      Evaluate(p0->r);
      assert(p0->r->data.type == GlyphType::NUMBER);
      p0->data.type = GlyphType::LINEAR_ENCODED_FORM;
      p0->data.lef = LEFEncodeNumber(p0->r->data.value);
      return p0;
    case FunctionType::DEMODULATE:
      Evaluate(p0->r);
      assert(p0->r->data.type == GlyphType::LINEAR_ENCODED_FORM);
      p0->data.type = GlyphType::NUMBER;
      p0->data.value = LEFDecodeNumber(p0->r->data.lef);
      return p0;
    case FunctionType::NEGATE:
      Evaluate(p0->r);
      assert(p0->r->data.type == GlyphType::NUMBER);
      p0->data.type = GlyphType::NUMBER;
      p0->data.value = -p0->r->data.value;
      return p0;
    case FunctionType::POWER_OF_TWO:
      Evaluate(p0->r);
      assert(p0->r->data.type == GlyphType::NUMBER);
      assert(p0->r->data.value >= 0);
      p0->data.type = GlyphType::NUMBER;
      p0->data.value = (1ll << p0->r->data.value);
      return p0;
    case FunctionType::I_COMBINATOR:
      if (p1) {
        p1->l = p0->r;
        return p1;
      } else {
        Evaluate(p0->r);
        *p0 = *p0->r;
        return p0;
      }
    case FunctionType::CAR__FIRST:
    case FunctionType::CDR__TAIL:
      p0->l = p0->r;
      p0->r = GetFromDictionary((node->data.ftype == FunctionType::CAR__FIRST)
                                    ? FunctionType::K_COMBINATOR
                                    : FunctionType::FALSE__SECOND);
      return p0;
    case FunctionType::NIL__EMPTY_LIST:
      p0->data.type = GlyphType::FUNCTION;
      p0->data.ftype = FunctionType::K_COMBINATOR;
      return p0;
    case FunctionType::IS_NIL:
      EvaluateLazyIsNil(p0->r);
      if ((p0->r->data.type == GlyphType::FUNCTION) &&
          (p0->r->data.ftype == FunctionType::NIL__EMPTY_LIST)) {
        p0->data.type = GlyphType::FUNCTION;
        p0->data.ftype = FunctionType::K_COMBINATOR;
      } else if ((p0->r->data.type == GlyphType::UP) &&
                 (p0->r->l->data.type == GlyphType::UP) &&
                 (p0->r->l->l->data.type == GlyphType::FUNCTION) &&
                 (p0->r->l->l->data.ftype == FunctionType::CONS__PAIR)) {
        p0->data.type = GlyphType::FUNCTION;
        p0->data.ftype = FunctionType::FALSE__SECOND;
      } else {
        assert(false);
      }
      return p0;
    case FunctionType::SUM:
      Evaluate(p0->r);
      Evaluate(p1->r);
      if ((p0->r->data.type == GlyphType::NUMBER) && (p0->r->data.value == 0)) {
        p1->data = p1->r->data;
      } else if ((p1->r->data.type == GlyphType::NUMBER) &&
                 (p1->r->data.value == 0)) {
        p1->data = p0->r->data;
      } else {
        assert(p0->r->data.type == GlyphType::NUMBER);
        assert(p1->r->data.type == GlyphType::NUMBER);
        p1->data.type = GlyphType::NUMBER;
        p1->data.value = p0->r->data.value + p1->r->data.value;
      }
      return p1;
    case FunctionType::PRODUCT:
      Evaluate(p0->r);
      if ((p0->r->data.type == GlyphType::NUMBER) && (p0->r->data.value == 0)) {
        p1->data.type = GlyphType::NUMBER;
        p1->data.value = 0;
        return p1;
      }
      Evaluate(p1->r);
      if ((p0->r->data.type == GlyphType::NUMBER) && (p0->r->data.value == 1)) {
        p1->data = p1->r->data;
      } else if ((p1->r->data.type == GlyphType::NUMBER) &&
                 (p1->r->data.value == 0)) {
        p1->data.type = GlyphType::NUMBER;
        p1->data.value = 0;
      } else if ((p1->r->data.type == GlyphType::NUMBER) &&
                 (p1->r->data.value == 1)) {
        p1->data = p0->r->data;
      } else {
        assert(p0->r->data.type == GlyphType::NUMBER);
        assert(p1->r->data.type == GlyphType::NUMBER);
        p1->data.type = GlyphType::NUMBER;
        p1->data.value = p0->r->data.value * p1->r->data.value;
      }
      return p1;
    case FunctionType::DIVISION:
      if (p0->r == p1->r) {
        p1->data.type = GlyphType::NUMBER;
        p1->data.value = 0;
        return p1;
      }
      Evaluate(p1->r);
      if ((p1->r->data.type == GlyphType::NUMBER) && (p1->r->data.value == 1)) {
        p1->data = p0->r->data;
      } else {
        Evaluate(p0->r);
        assert(p0->r->data.type == GlyphType::NUMBER);
        assert(p1->r->data.type == GlyphType::NUMBER);
        p1->data.type = GlyphType::NUMBER;
        p1->data.value = p0->r->data.value / p1->r->data.value;
      }
      return p1;
    case FunctionType::EQUALITY:
      if (p0->r == p1->r) {
        p1->data.type = GlyphType::FUNCTION;
        p1->data.ftype = FunctionType::K_COMBINATOR;
      } else {
        Evaluate(p0->r);
        Evaluate(p1->r);
        if ((p0->r->data.type == GlyphType::VARIABLE) &&
            (p1->r->data.type == GlyphType::VARIABLE) &&
            (p0->r->data.value == p1->r->data.value)) {
          p1->data.type = GlyphType::FUNCTION;
          p1->data.ftype = FunctionType::K_COMBINATOR;
        } else {
          assert(p0->r->data.type == GlyphType::NUMBER);
          assert(p1->r->data.type == GlyphType::NUMBER);
          p1->data.type = GlyphType::FUNCTION;
          p1->data.ftype = (p0->r->data.value == p1->r->data.value)
                               ? FunctionType::K_COMBINATOR
                               : FunctionType::FALSE__SECOND;
        }
      }
      return p1;
    case FunctionType::STRICT_LESS:
      if (p0->r == p1->r) {
        p1->data.type = GlyphType::FUNCTION;
        p1->data.ftype = FunctionType::FALSE__SECOND;
      } else {
        Evaluate(p0->r);
        Evaluate(p1->r);
        assert(p0->r->data.type == GlyphType::NUMBER);
        assert(p1->r->data.type == GlyphType::NUMBER);
        p1->data.type = GlyphType::FUNCTION;
        p1->data.ftype = (p0->r->data.value < p1->r->data.value)
                             ? FunctionType::K_COMBINATOR
                             : FunctionType::FALSE__SECOND;
      }
      return p1;
    case FunctionType::K_COMBINATOR:
      p1->r = p0->r;
      p1->l = GetFromDictionary(FunctionType::I_COMBINATOR);
      return p1;
    case FunctionType::FALSE__SECOND:
      p1->l = GetFromDictionary(FunctionType::I_COMBINATOR);
      return p1;
    case FunctionType::S_COMBINATOR: {
      auto n1 = NewNode(GlyphType::UP);
      auto n2 = NewNode(GlyphType::UP);
      n1->l = p0->r;
      n1->r = p2->r;
      n2->l = p1->r;
      n2->r = p2->r;
      p2->l = n1;
      p2->r = n2;
      return p2;
    }
    case FunctionType::C_COMBINATOR: {
      auto n1 = NewNode(GlyphType::UP);
      n1->l = p0->r;
      n1->r = p2->r;
      p2->l = n1;
      p2->r = p1->r;
      return p2;
    }
    case FunctionType::B_COMBINATOR: {
      auto n1 = NewNode(GlyphType::UP);
      n1->l = p1->r;
      n1->r = p2->r;
      p2->l = p0->r;
      p2->r = n1;
      return p2;
    }
    case FunctionType::CONS__PAIR:
    case FunctionType::VECTOR: {
      auto n1 = NewNode(GlyphType::UP);
      n1->l = p2->r;
      n1->r = p0->r;
      p2->l = n1;
      p2->r = p1->r;
      return p2;
    }
    case FunctionType::IF0:
      Evaluate(p0->r);
      assert(p0->r->data.type == GlyphType::NUMBER);
      if (p0->r->data.value == 0) p2->r = p1->r;
      p2->l = GetFromDictionary(FunctionType::I_COMBINATOR);
      return p2;
    default:
      assert(false);
      return nullptr;
  }
}

Node* EvaluateI(Node* node, std::vector<Node*>& current_path) {
  bool subtree_changed = false;
  while (true) {
    assert(node);
    if (node->data.type == GlyphType::ALIAS) {
      subtree_changed = true;
      ExpandAlias(node);
    } else if (node->data.type == GlyphType::UP) {
      current_path.push_back(node);
      Node* pnext = EvaluateI(node->l, current_path);
      current_path.pop_back();
      if (pnext == node) {
        subtree_changed = true;
      } else if (!pnext) {
        Evaluate(node->r);
        break;
      } else {
        return pnext;
      }
    } else if (node->data.type == GlyphType::FUNCTION) {
      if (current_path.size() >= ExpectedParameters(node->data.ftype)) {
        subtree_changed = true;
        Node* pnext = ApplyFunction(node, current_path);
        if (pnext != node) return pnext;
      } else {
        break;
      }
    } else {
      break;
    }
  }
  return subtree_changed ? GetI(current_path, 0) : nullptr;
}
}  // namespace

void ExpandAlias(Node* node) {
  assert(node && node->data.type == GlyphType::ALIAS);
  node->l = GetFromDictionary(FunctionType::I_COMBINATOR);
  node->r = GetFromDictionary(node->data.value);
  node->data.type = GlyphType::UP;
}

void EvaluateLazyIsNil(Node* node) {
  assert(node);
  std::vector<Node*> v;
  v.push_back(node);
  for (;;) {
    if (node->data.type == GlyphType::ALIAS) {
      ExpandAlias(node);
    } else if (node->data.type == GlyphType::UP) {
      auto l = node->l;
      if (l->data.type == GlyphType::ALIAS) {
        ExpandAlias(l);
      } else if (l->data.type == GlyphType::FUNCTION) {
        if (ExpectedParameters(l->data.ftype) <= 1) {
          ApplyFunction(l, v);
        } else {
          break;
        }
      } else if (l->data.type == GlyphType::UP) {
        auto l2 = l->l;
        if (l2->data.type == GlyphType::ALIAS) {
          ExpandAlias(l2);
        } else if (l2->data.type == GlyphType::FUNCTION) {
          if (ExpectedParameters(l2->data.ftype) <= 2) {
            v.push_back(l);
            ApplyFunction(l2, v);
            v.pop_back();
          } else {
            break;
          }
        } else if (l2->data.type == GlyphType::UP) {
          v.push_back(l);
          EvaluateI(l2, v);
          v.pop_back();
        } else {
          break;
        }
      } else {
        break;
      }
    } else {
      break;
    }
  }
}

void Evaluate(Node* node) {
  assert(node);
  std::vector<Node*> v;
  EvaluateI(node, v);
  assert(v.size() == 0);
}