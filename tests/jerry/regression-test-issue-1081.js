// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

try { new RegExp().compile(A)  } catch (err) {}
try { A } catch (err) {}
try { new RegExp().constructor.prototype.toString()  } catch (A) {}
try { A } catch (err) {}
try { new RegExp(1, "g").exec(1)  } catch (A) {}
try { A } catch (err) {}
try { A } catch (A) {}
try { A } catch (A) {}
try { new (Boolean.constructor.prototype)  } catch (err) {}
try { Boolean  } catch (A) {}
try { A } catch(A) {}
try { new RegExp().compile(new (RegExp)())  } catch (A) {}
try { Math.max().constructor.seal(A) } catch (err) {}
try { Date.parse(RegExp.prototype.compile(RegExp.prototype)) } catch(A) {}
try { Boolean.A() } catch (err) {}
