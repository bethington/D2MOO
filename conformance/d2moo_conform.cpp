// ============================================================================
//  d2moo_conform - PD2-S12 conformance harness for D2MOO
//
//  Replays golden vectors MINTED FROM THE REAL PD2-S12 BINARY (via the ghidra-mcp
//  in-process call_function oracle) against D2MOO's actual reimplementation, and
//  asserts they match bit-for-bit. This is the piece D2MOO lacks: automated
//  conformance against a specific target binary (1.13c / Project Diablo 2 S12).
//
//  It links D2MOO's SHIPPING headers (e.g. D2Seed.h) so a pass proves the real
//  drop-in code conforms. Where a function diverges, D2MOO's 1.10f-derived logic
//  needs a 1.13c fix. Data-driven: add a vectors/*.json file + a dispatch case.
//
//  Build (MSVC, native — i64 literals work):  compile with D2Common/include on the
//  include path. Quick check on g++/clang: see conformance/build.sh (normalizes the
//  MSVC i64 suffix + stubs D2BasicTypes.h/D2Common.h).
//
//  Usage:  d2moo_conform [vectors/rng.json ...]   (defaults to vectors/rng.json)
//  Exit 0 = all pass.
// ============================================================================
#include "D2Seed.h"     // D2MOO's real RNG (SEED_RollLimitedRandomNumber)

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

// --- tiny JSON reader (objects/arrays/ints/strings/bool) for machine vectors ---
struct JVal {
	enum T { NUL, BOOL, NUM, STR, ARR, OBJ } type = NUL;
	long long num = 0; bool b = false; std::string str;
	std::vector<JVal> arr; std::map<std::string, JVal> obj;
	const JVal* find(const char* k) const { auto it = obj.find(k); return it == obj.end() ? nullptr : &it->second; }
	long long n(const char* k, long long d = 0) const { const JVal* v = find(k); return v ? v->num : d; }
	bool has(const char* k) const { return find(k) != nullptr; }
};
struct JP {
	const char* p; const char* e;
	JP(const std::string& s) : p(s.c_str()), e(s.c_str() + s.size()) {}
	void ws() { while (p < e && (*p==' '||*p=='\t'||*p=='\n'||*p=='\r')) ++p; }
	JVal val() { ws(); if (p>=e) return {}; char c=*p;
		if (c=='{') return obj(); if (c=='[') return arr();
		if (c=='"') { JVal v; v.type=JVal::STR; v.str=str(); return v; }
		if (c=='t'||c=='f') { JVal v; v.type=JVal::BOOL; v.b=(*p=='t'); p+=(*p=='t'?4:5); return v; }
		if (c=='n') { p+=4; return {}; } return num(); }
	JVal obj() { JVal v; v.type=JVal::OBJ; ++p; ws(); if (p<e&&*p=='}'){++p;return v;}
		while (p<e){ ws(); std::string k=str(); ws(); if(p<e&&*p==':')++p; v.obj[k]=val(); ws();
			if(p<e&&*p==','){++p;continue;} if(p<e&&*p=='}'){++p;break;} break; } return v; }
	JVal arr() { JVal v; v.type=JVal::ARR; ++p; ws(); if (p<e&&*p==']'){++p;return v;}
		while (p<e){ v.arr.push_back(val()); ws();
			if(p<e&&*p==','){++p;continue;} if(p<e&&*p==']'){++p;break;} break; } return v; }
	std::string str() { std::string s; if(p<e&&*p=='"')++p;
		while(p<e&&*p!='"'){ if(*p=='\\'&&p+1<e){++p;s.push_back(*p++);} else s.push_back(*p++);} if(p<e&&*p=='"')++p; return s; }
	JVal num() { JVal v; v.type=JVal::NUM; const char* s=p;
		while(p<e&&(*p=='-'||*p=='+'||(*p>='0'&&*p<='9')||*p=='.'||*p=='e'||*p=='E'))++p;
		v.num=(long long)strtoll(std::string(s,p).c_str(),nullptr,10); return v; }
};

static bool run_case(const JVal& c, int idx, const std::string& file) {
	const JVal* fnv=c.find("fn"); const JVal* in=c.find("in"); const JVal* out=c.find("out");
	if (!fnv||!in||!out) { printf("  FAIL %s[%d]: malformed\n", file.c_str(), idx); return false; }
	const std::string& fn = fnv->str;

	// D2Seed_Random -> D2MOO SEED_RollLimitedRandomNumber (D2Common/include/D2Seed.h)
	if (fn == "D2Seed_Random" || fn == "D2Seed_RandomInRange") {
		D2SeedStrc s; s.nLowSeed=(uint32_t)in->n("seedLo"); s.nHighSeed=(uint32_t)in->n("seedHi");
		uint32_t got;
		if (fn == "D2Seed_Random")
			got = SEED_RollLimitedRandomNumber(&s, (int)in->n("max"));
		else { // D2MOO's range idiom: nMin + SEED_RollLimitedRandomNumber(&s, nMax-nMin+1)
			int32_t mn=(int32_t)in->n("min"), mx=(int32_t)in->n("max");
			got = (uint32_t)(mn + (int32_t)SEED_RollLimitedRandomNumber(&s, mx - mn + 1));
		}
		uint32_t exp=(uint32_t)out->n("ret");
		bool ok=(got==exp) && !(out->has("newHi") && s.nHighSeed!=(uint32_t)out->n("newHi"));
		if (!ok) printf("  FAIL %s[%d] %s: got ret=%u newHi=%08x exp ret=%u newHi=%08x\n",
			file.c_str(), idx, fn.c_str(), got, s.nHighSeed, exp, (uint32_t)out->n("newHi"));
		return ok;
	}
	printf("  FAIL %s[%d]: unknown fn '%s'\n", file.c_str(), idx, fn.c_str());
	return false;
}

static bool run_file(const std::string& path) {
	std::ifstream f(path, std::ios::binary);
	if (!f) { printf("  ERROR: cannot open '%s'\n", path.c_str()); return false; }
	std::stringstream ss; ss << f.rdbuf(); std::string text = ss.str();
	JP jp(text); JVal root = jp.val();
	if (root.type != JVal::ARR) { printf("  ERROR: '%s' not a JSON array\n", path.c_str()); return false; }
	std::string base = path; size_t sl = base.find_last_of("/\\"); if (sl!=std::string::npos) base = base.substr(sl+1);
	int pass=0, fail=0;
	for (size_t i=0;i<root.arr.size();++i) { if (run_case(root.arr[i],(int)i,base)) ++pass; else ++fail; }
	printf("  %-16s %3d/%d passed%s\n", base.c_str(), pass, pass+fail, fail?"   <<< FAIL":"");
	return fail==0;
}

int main(int argc, char** argv) {
	std::vector<std::string> files;
	if (argc>1) for (int i=1;i<argc;++i) files.push_back(argv[i]);
	else files.push_back("vectors/rng.json");
	printf("d2moo_conform - D2MOO vs PD2-S12 golden vectors\n");
	bool all=true; for (auto& f: files) if (!run_file(f)) all=false;
	printf("%s\n", all ? "ALL D2MOO CONFORMANCE VECTORS PASSED" : "CONFORMANCE FAILURE");
	return all ? 0 : 1;
}
