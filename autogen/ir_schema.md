# Data Dictionary

### `MaroonIRVar`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `name` | String |
| `type` | String |
| `init` | `null` or String |


### `MaroonIRStmt`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `stmt` | String |


### `MaroonIRIf`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `cond` | String |
| `yes` | Algebraic `MaroonIRStmt` / `MaroonIRIf` / `MaroonIRBlock` / `MaroonIRBlockPlaceholder` (a.k.a. `MaroonIRStmtOrBlock`) |
| `no` | Algebraic `MaroonIRStmt` / `MaroonIRIf` / `MaroonIRBlock` / `MaroonIRBlockPlaceholder` (a.k.a. `MaroonIRStmtOrBlock`) |


### `MaroonIRBlockPlaceholder`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `_idx` | Integer (32-bit unsigned) |


### `MaroonIRStmtOrBlock`
Algebraic type, `MaroonIRStmt` or `MaroonIRIf` or `MaroonIRBlock` or `MaroonIRBlockPlaceholder`


### `MaroonIRBlock`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `vars` | Array of `MaroonIRVar` |
| `code` | Array of Algebraic `MaroonIRStmt` / `MaroonIRIf` / `MaroonIRBlock` / `MaroonIRBlockPlaceholder` (a.k.a. `MaroonIRStmtOrBlock`) |


### `MaroonIRFunction`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `args` | Array of String |
| `body` | `MaroonIRBlock` |


### `MaroonIRFiber`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `functions` | Ordered map of String into `MaroonIRFunction` |


### `MaroonIRTypeDefStructField`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `name` | String |
| `type` | String |


### `MaroonIRTypeDefStruct`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `fields` | Array of `MaroonIRTypeDefStructField` |


### `MaroonIRTypeDef`
Algebraic type, `MaroonIRTypeDefStruct`


### `MaroonIRType`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `def` | Algebraic `MaroonIRTypeDefStruct` (a.k.a. `MaroonIRTypeDef`) |


### `MaroonIRNamespace`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `fibers` | Ordered map of String into `MaroonIRFiber` |
| `types` | Ordered map of String into `MaroonIRType` |


### `MaroonTestCaseRunFiber`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `maroon` | String |
| `fiber` | String |
| `golden_output` | Array of String |


### `MaroonTestCaseFiberShouldThrow`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `line` | Integer (32-bit unsigned) |
| `maroon` | String |
| `fiber` | String |
| `error` | String |


### `MaroonTestCase`
Algebraic type, `MaroonTestCaseRunFiber` or `MaroonTestCaseFiberShouldThrow`


### `MaroonIRScenarios`
| **Field** | **Type** | **Description** |
| ---: | :--- | :--- |
| `src` | String | The source `.mrn` file. |
| `maroon` | Ordered map of String into `MaroonIRNamespace` |
| `tests` | Array of Algebraic `MaroonTestCaseRunFiber` / `MaroonTestCaseFiberShouldThrow` (a.k.a. `MaroonTestCase`) |

