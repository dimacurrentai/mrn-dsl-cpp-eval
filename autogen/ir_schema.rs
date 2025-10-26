#![allow(unused_imports)]
use serde::{Deserialize, Serialize};
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRVar {
  pub line: u32,
  pub name: String,
  pub r#type: String,
  pub init: Option<String>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRStmt {
  pub line: u32,
  pub stmt: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRIf {
  pub line: u32,
  pub cond: String,
  pub yes: Box<MaroonIRStmtOrBlock>,
  pub no: Box<MaroonIRStmtOrBlock>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRBlockPlaceholder {
  pub line: u32,
  pub _idx: u32,
}

#[derive(Debug, Serialize, Deserialize)]
pub enum MaroonIRStmtOrBlock {
  MaroonIRStmt(MaroonIRStmt),
  MaroonIRIf(MaroonIRIf),
  MaroonIRBlock(MaroonIRBlock),
  MaroonIRBlockPlaceholder(MaroonIRBlockPlaceholder),
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRBlock {
  pub line: u32,
  pub vars: Vec<MaroonIRVar>,
  pub code: Vec<MaroonIRStmtOrBlock>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRFunction {
  pub line: u32,
  pub ret: Option<String>,
  pub args: Vec<String>,
  pub body: MaroonIRBlock,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRFiber {
  pub line: u32,
  pub functions: BTreeMap<String, MaroonIRFunction>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRTypeDefStructField {
  pub name: String,
  pub r#type: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRTypeDefStruct {
  pub fields: Vec<MaroonIRTypeDefStructField>,
}

#[derive(Debug, Serialize, Deserialize)]
pub enum MaroonIRTypeDef {
  MaroonIRTypeDefStruct(MaroonIRTypeDefStruct),
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRType {
  pub line: u32,
  pub def: Box<MaroonIRTypeDef>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRNamespace {
  pub line: u32,
  pub fibers: BTreeMap<String, MaroonIRFiber>,
  pub types: BTreeMap<String, MaroonIRType>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonTestCaseRunFiber {
  pub line: u32,
  pub maroon: String,
  pub fiber: String,
  pub golden_output: Vec<String>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonTestCaseFiberShouldThrow {
  pub line: u32,
  pub maroon: String,
  pub fiber: String,
  pub error: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub enum MaroonTestCase {
  MaroonTestCaseRunFiber(MaroonTestCaseRunFiber),
  MaroonTestCaseFiberShouldThrow(MaroonTestCaseFiberShouldThrow),
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MaroonIRScenarios {
  // The source `.mrn` file.
  pub src: String,
  pub maroon: BTreeMap<String, MaroonIRNamespace>,
  pub tests: Vec<MaroonTestCase>,
}
