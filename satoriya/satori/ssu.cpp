//------------------------------------------------
//
//	里々同梱ユーティリティライブラリ　ssu.dll
//
#include    "random.h"
#include    "ssu.h"
#include	<map>
#include	<algorithm>
#include	<time.h>

#ifdef _WINDOWS
#include <mbctype.h>
#endif

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

//================================================================================================

#include	"SaoriHost.h"

static SRV	call_ssu(string iCommand, std::deque<string>& iArguments, std::deque<string>& oValues);

#ifndef SSU_SAORI_CALL_INTERFACE

//SaoriClientに直結するインターフェース

//要らないインターフェースは潰しておく
bool ssu::load(
	const string& i_sender,
	const string& i_charset,
	const string& i_work_folder,
	const string& i_dll_fullpath)
{
	return true;
}
void ssu::unload()
{
	//NOOP
}
string ssu::request(const string& i_request_string)
{
	return "";
}
string ssu::get_version(const string& i_security_level)
{
	return "SAORI/1.0";
}

int ssu::request(
		const std::vector<string>& i_argument,
		bool i_is_secure,
		string& o_result,
	std::vector<string>& o_value)
{
	if ( i_argument.size()<1 ) {
		o_result = "命令が指定されていません";
		return 400;
	}

	// 最初の引数は命令名として扱う
	std::vector<string>::const_iterator i_arg = i_argument.begin();
	string theCommand = *i_arg;
	++i_arg;

	std::deque<string> iArguments;
	for ( ; i_arg != i_argument.end() ; ++i_arg ) {
		iArguments.push_back(*i_arg);
	}

	std::deque<string> oValues;

	SRV result = call_ssu(theCommand, iArguments, oValues);
	o_result = result.mResultString;

	// 注意！Valueヘッダ相当が存在しないときは、S?系システム変数を温存するためにデータを上書きしないこと！
	if ( ! oValues.empty() ) {
		o_value.clear();
		for (std::deque<string>::const_iterator o_val = oValues.begin() ; o_val != oValues.end() ; ++o_val ) {
			o_value.push_back(*o_val);
		}
	}

	return result.mReturnCode;
}

#else

//通常のSSU(SAORI版)
//ビルド時 SSU_SAORI_CALL_INTERFACE を定義すると有効になる。SSU単独ビルド時に定義すること。

class ssu : public SaoriHost {
public:
	ssu() {
		randomize();
	}
	virtual bool	load(const string& i_base_folder) {
		setlocale(LC_ALL, "Japanese");
	#ifdef _WINDOWS
		_setmbcp(_MB_CP_LOCALE);
	#endif
		return true;
	}
	virtual SRV		request(std::deque<string>& iArguments, std::deque<string>& oValues);
};
SakuraDLLHost* SakuraDLLHost::m_dll = new ssu;

SRV	ssu::request(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<1 )
		return	SRV(400, "命令が指定されていません");

	// 最初の引数は命令名として扱う
	string	theCommand = iArguments.front();
	iArguments.pop_front();
	return	call_ssu(theCommand, iArguments, oValues);
}

#endif

//================================================================================================

typedef SRV (*Command)(std::deque<string>&, std::deque<string>&);

static const std::map<string, Command> &func_map(void)
{
	// 名前と命令を関連付けたmap
	static std::map<string, Command>	theMap;
	if ( theMap.empty() )
	{ 
		// 初回準備
		#define	d(iName)	\
			SRV	_##iName(std::deque<string>&, std::deque<string>&); \
			theMap[ #iName ] = _##iName
		// 命令一覧の宣言と関連付け。
		d(calc);			d(calc_float);		d(if);				d(unless);
		d(nswitch);			d(switch);			d(iflist);			d(substr);
		d(split);			d(split_string);	d(join);
		d(replace);			d(replace_first);	d(erase);
		d(erase_first);		d(count);
		d(compare);				d(compare_head);			d(compare_tail);
		d(compare_case);		d(compare_head_case);		d(compare_tail_case);
		d(length);			d(is_empty);		d(is_digit);
		d(is_alpha);		d(zen2han);			d(han2zen);			d(hira2kata);
		d(kata2hira);		d(sprintf);			d(reverse);			d(at);
		d(choice);
		d(lsimg);			d(mkdir);
		#undef	d
	}
	return theMap;
}

void get_ssu_funclist(std::vector<string> &funclist)
{
	const std::map<string, Command> &theMap = func_map();

	funclist.clear();

	for (std::map<string, Command>::const_iterator i = theMap.begin() ; i != theMap.end() ; ++i ) {
		funclist.push_back(i->first);
	}
}

static SRV	call_ssu(string iCommand, std::deque<string>& iArguments, std::deque<string>& oValues)
{
	const std::map<string, Command> &theMap = func_map();

	// 命令の存在を確認
	std::map<string, Command>::const_iterator i = theMap.find(iCommand);
	if ( i==theMap.end() )
		return SRV(400, string()+"Error: '"+iCommand+"'という名前の命令は定義されていません。");

	// 実際に呼ぶ
	return	i->second(iArguments, oValues);
}

// ここから実装

#ifdef POSIX
#  include      "../_/Utilities.h"
#  include <sys/stat.h>
#  include <sys/types.h>
#else
#  include	<windows.h>
#endif
#include	"../_/stltool.h"

/* 「ソ」の2バイト目は「\」であるので、エスケープする必要がある。 */
//static const char	zen[] = 
//	"　ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ"
//	"０１２３４５６７８９！”＃＄％＆’（）＝〜｜‘｛＋＊｝＜＞？＿ー＾￥＠「；：」、。・÷×−，．［］"
//	"アイウエオカキクケコサシスセ\x83\x5cタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンァィゥェォャュョッ゛゜、。";
//static const char	han[] = 
//	" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
//	"0123456789!\"#$%&'()=~|`{+*}<>?_-^\\@[;:],.･/*-,.[]"
//	"ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝｧｨｩｪｫｬｭｮｯﾞﾟ､｡";
static const char	kata[] = "アイウエオカキクケコサシスセ\x83\x5cタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヰヱヲンァィゥェォャュョヮッガギグゲゴザジズゼゾダヂヅデドバビブベボパピプペポ";
static const char	hira[] = "あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわゐゑをんぁぃぅぇぉゃゅょゎっがぎぐげござじずぜぞだぢづでどばびぶべぼぱぴぷぺぽ";

//半角全角変換テーブル
static const char	zen_alpha[] = "ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ";
static const char	han_alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static const char	zen_digit[] = "０１２３４５６７８９";
static const char	han_digit[] = "0123456789";

static const char   zen_symbol[] = "　！”＃＄％＆’（）＝〜｜‘｛＋＊｝＜＞？＿ー＾￥＠「；：」、。・÷×−，．［］";
static const char   han_symbol[] = " !\"#$%&'()=~|`{+*}<>?_-^\\@[;:],.･/*-,.[]";

static const char   zen_kana_1[] = "アイウエオカキクケコサシスセ\x83\x5cタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンァィゥェォャュョッ、。ー";
static const char   han_kana_1[] = "ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝｧｨｩｪｫｬｭｮｯ､｡ｰ";

static const char   zen_kana_2[] = "ガギグゲゴザジズゼゾダヂヅデドバビブベボパピプペポ";
static const char   han_kana_2[] = "ｶﾞｷﾞｸﾞｹﾞｺﾞｻﾞｼﾞｽﾞｾﾞｿﾞﾀﾞﾁﾞﾂﾞﾃﾞﾄﾞﾊﾞﾋﾞﾌﾞﾍﾞﾎﾞﾊﾟﾋﾟﾌﾟﾍﾟﾎﾟ";

extern	bool calc(string& ioString,bool isStrict = false);
extern	bool calc_float(string& ioString);

static string zen2han_internal(string &str,unsigned int flag = 0xffffU);

#include	<sstream>

// 半角/全角を同等に扱った上で文字長を返す
int	sjis_strlen(const char* p) {
	int	n=0;
	for (int i=0 ; p[i] != '\0' ; i += _ismbblead(p[i]) ? 2 : 1 )
		++n;
	return	n;
}

// 半角/全角を同等に扱った上でn文字移動、超過時はNULL
const char*	sjis_at(const char* p, int n) {
	for (int i=0 ; i<n ; ++i) {
		if ( *p == '\0' )
			return	NULL;
		p += _ismbblead(*p) ? 2 : 1;
	}
	return	p;
}

bool	printf_format(const char*& p, std::deque<string>& iArguments, std::stringstream& os)
{
	assert(*p=='%');
	if ( iArguments.empty() )
		return	false;	// 置き換え対象が無い

	++p;
	string	str = iArguments.front();
	iArguments.pop_front();

	// フラグ指定読み込み
	bool isSharp=false;

	while (true) {
		if ( *p == '-' ) { os << std::left; ++p; }
		else if ( *p == '+' ) { os << std::showpos; ++p; }
		else if ( *p == '0' ) { os.fill('0'); os<< std::internal; ++p; }
		else if ( *p == ' ' ) { os.fill(' '); os<< std::internal; ++p; }
		else if ( *p == '#' ) { isSharp = true; ++p; }
		else break;
	}

	// 幅指定読み込み
	int	width=0;
	bool	isReadWidth = false;
	if ( *p=='*' ) {
		isReadWidth = true;
		++p;
	} else {
		while ( *p>='0' && *p<='9' ) {
			width = width*10 + (*p - '0');
			++p;
			os.width(width);
		}
	}

	// 精度指定読み込み
	int	precision = 0;
	if ( *p == '.' ) {
		++p;
		while ( *p>='0' && *p<='9' ) {
			precision = precision*10 + (*p - '0');
			++p;
		}
		os.precision(precision);
	}

	// フォーマット設定 - #フラグ
	if ( isSharp ) {
		switch (*p) {
			case 'o':
			case 'x':
			case 'X':
				os << std::showbase;
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				os << std::showpoint;
				break;
		}
	}
	

	// 変換文字に応じて挿入
	switch (*p) {
	case 's':
	case 'S':
		{
			os << str;
			break;
		}
	case 'c':
	case 'C':
		{
			os << (char)zen2int(str);
			break;
		}
	case 'd':
		{
			os << zen2int(str);
			break;
		}
	case 'i':
		{
			os << std::oct << zen2int(str);
			break;
		}
	case 'o':
		{
			os << std::oct << strtoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case 'u':
		{
			os << strtoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case 'x':
		{
			os << std::hex << std::nouppercase << strtoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case 'X':
		{
			os << std::hex << std::uppercase << strtoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case 'e':
		{
			os << std::scientific << std::nouppercase << strtod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case 'E':
		{
			os << std::scientific << std::uppercase << strtod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case 'g':
		{
			os << std::scientific << std::fixed << std::nouppercase << strtod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case 'G':
		{
			os << std::scientific << std::fixed << std::uppercase << strtod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case 'f':
		{
			os << std::fixed << strtod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case 'n': break;
	case 'p': break;
	default: return false;
	}
	++p;
	return	true;
}

string	sprintf(std::deque<string>& iArguments) {
	std::stringstream s;
	string	str = iArguments.front();
	iArguments.pop_front();
	const char* p = str.c_str();
	while ( *p!='\0' ) {
		if ( *p=='%' ) {
			std::stringstream sf;
			if ( printf_format(p, iArguments, sf) ) {
				s << sf.str();
				continue;
			}
		}
		if ( _ismbblead(*p) ) {
			s << *p++; s << *p++;
		} else {
			s << *p++;
		}
	}
	return	s.str();
}


SRV _calc(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, "引数の個数が正しくありません。");
	string	exp = iArguments[0];
	if ( !calc(exp) )
		return	SRV(400, string()+"'"+iArguments[0]+"' 式が計算不\x94\x5cです。"); // 「能」の2バイト目は「\」
	return	exp;
}

SRV _calc_float(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, "引数の個数が正しくありません。");
	string	exp = iArguments[0];
	if ( !calc_float(exp) )
	    return	SRV(400, string()+"'"+iArguments[0]+"' 式が計算不\x94\x5cです。");
	return	exp;
}

SRV _if(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<2 || iArguments.size()>3 )
		return	SRV(400, "引数の個数が正しくありません。");
	string	exp = iArguments[0];
	if ( !calc(exp) )
		return	SRV(400, string()+"'"+iArguments[0]+"' 式が計算不\x94\x5cです。");
	if ( zen2int(exp) != 0 )
		return	iArguments[1];	// 真
	else
		if ( iArguments.size()==3 )
			return	iArguments[2];	// 偽
		else
			return	SRV(204);	// 偽でelseなし
}

SRV _unless(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<2 || iArguments.size()>3 )
		return	SRV(400, "引数の個数が正しくありません。");
	string	exp = iArguments[0];
	if ( !calc(exp) )
		return	SRV(400, string()+"'"+iArguments[0]+"' 式が計算不\x94\x5cです。");
	if ( zen2int(exp) == 0 )
		return	iArguments[1];	// 偽
	else
		if ( iArguments.size()==3 )
			return	iArguments[2];	// 真
		else
			return	SRV(204);	// 真でelseなし
}

SRV _nswitch(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, "引数が足りません。");

	int	n = zen2int(iArguments[0]);
	//iArguments.pop_front();
	//if ( iArguments.size()>n )
	if ( n>0 && iArguments.size()>n )
		return	SRV(200, iArguments[n]);
	else
		return	SRV(204);
}

SRV _switch(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, "引数が足りません。");

	const string	lhs = iArguments[0];
	const int max = iArguments.size();
	for (int i=1 ; i<max ; i+=2) {
		if ( i==max-1 ) // 引数が奇数個の場合、最後の１つはelse式
			return	SRV(200, iArguments[i]);
		string	exp = string("(") + lhs + ")==(" + iArguments[i] + ")";
		if ( !calc(exp) )
			return	SRV(400, string()+"switchの"+itos((i-1)/2+1)+"個目、式 '"+exp+"' は計算不\x94\x5cでした。");
		if ( zen2int(exp) != 0 )
			return	SRV(200, iArguments[i+1]);
	}
	return	SRV(204);
}

SRV _iflist(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, "引数が足りません。");

	const string	lhs = iArguments[0];
	const int max = iArguments.size();
	for (int i=1 ; i<max ; i+=2) {
		if ( i==max-1 ) // 引数が奇数個の場合、最後の１つはelse扱い。ここまできたら無条件でそれを返す。
			return	SRV(200, iArguments[i]);
		string	exp = lhs + iArguments[i];
		if ( !calc(exp) )
			return	SRV(400, string()+"iflistの"+itos((i-1)/2+1)+"個目、式 '"+exp+"' は計算不\x94\x5cでした。");
		if ( zen2int(exp) != 0 )
			return	SRV(200, iArguments[i+1]);
	}
	return	SRV(204);
}


SRV _substr(std::deque<string>& iArguments, std::deque<string>& oValues) {

	if ( iArguments.size()<1 )
		return	SRV(400, "引数が足りません。");

	// 対象文字列
	const char* p = iArguments[0].c_str();
	if ( iArguments.size()==1 )
		return	SRV(200, p); // 引数１個なら全体を返す

	const int	len = sjis_strlen(p);

	// 始点
	int	start = zen2int(iArguments[1]);
	if ( start < 0 )
		start = len + start;

	// 始点からのオフセット値
	int offset = (iArguments.size()<=2) ? len : zen2int(iArguments[2]);
	if ( offset==0 || offset==INT_MIN ) // INT_MINの時は符号反転が効かないので0扱い。
		return	SRV(204);
	if ( offset<0 ) {
		start += offset;
		offset = -offset;
	}
	assert(offset >= 0 );

	if ( start < 0 )
		start = 0;
	if ( start >= len )
		return	SRV(204);
	if ( start + offset >= len )
		offset = len - start;

	const char* const start_p = sjis_at(p, start);
	const char* const end_p = sjis_at(start_p, offset);
	return	SRV(200, string(start_p, end_p));
}

SRV _split(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size() < 1 )
		return	SRV(400, "引数の個数が正しくありません。");

	strvec	vec;
	if ( iArguments.size()==1 ) {
		split(iArguments[0],vec);
	}
	else {
		int max_words = 0;
		if ( iArguments.size() > 2 ) {
			max_words = zen2int(iArguments[2]);
		}

		bool split_one = false;
		if ( iArguments.size() > 3 ) {
			split_one = zen2int(iArguments[3]) != 0;
		}

		split(iArguments[0].c_str(),iArguments[1].c_str(),vec,max_words,split_one);
	}

	for ( strvec::iterator i=vec.begin() ; i!=vec.end() ; ++i )
		oValues.push_back(*i);
	return	SRV(200, itos(vec.size()));
}

SRV _split_string(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size() < 1 )
		return	SRV(400, "引数の個数が正しくありません。");

	strvec	vec;
	if ( iArguments.size()==1 ) {
		split(iArguments[0],vec);
	}
	else {
		int max_words = 0;
		if ( iArguments.size() > 2 ) {
			max_words = zen2int(iArguments[2]);
		}

		bool split_one = false;
		if ( iArguments.size() > 3 ) {
			split_one = zen2int(iArguments[3]) != 0;
		}

		split_string(iArguments[0].c_str(),iArguments[1].c_str(),vec,max_words,split_one);
	}

	for ( strvec::iterator i=vec.begin() ; i!=vec.end() ; ++i )
		oValues.push_back(*i);
	return	SRV(200, itos(vec.size()));
}

SRV _join(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<1 )
		return	SRV(400, "引数の個数が正しくありません。");

	string	r = iArguments[1];
	for (int n=2 ; n<iArguments.size() ; ++n)
		r += iArguments[0] + iArguments[n];
	return	r;
}

SRV _replace(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=3 )
		return	SRV(400, "引数の個数が正しくありません。");
	replace(iArguments[0], iArguments[1], iArguments[2]);
	return	SRV(200, iArguments[0]);
}

SRV _replace_first(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=3 )
		return	SRV(400, "引数の個数が正しくありません。");
	replace_first(iArguments[0], iArguments[1], iArguments[2]);
	return	iArguments[0];
}

SRV _erase(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	erase(iArguments[0], iArguments[1]);
	return	iArguments[0];
}

SRV _erase_first(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	erase_first(iArguments[0], iArguments[1]);
	return	iArguments[0];
}

SRV _count(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size() >= 2 ) {
		int cnt = 0;

		int argsize = iArguments.size();

		for ( int arg = 0 ; arg < (argsize-1) ; ++arg ) {
			cnt += count(iArguments[arg], iArguments[argsize-1]);
		}

		return	itos( cnt );
	}
	else if ( iArguments.size()==1 ) { //arg0が空っぽで切り詰められた
		return  "0";
	}
	else {
		return	SRV(400, "引数の個数が正しくありません。");
	}
}

SRV _compare_case(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	return	(strcmp(zen2han_internal(iArguments[0]).c_str(), zen2han_internal(iArguments[1]).c_str())==0) ? "1" : "0";
}

SRV _compare(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	return	(stricmp(zen2han_internal(iArguments[0]).c_str(), zen2han_internal(iArguments[1]).c_str())==0) ? "1" : "0";
}

SRV _compare_head_case(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	return	compare_head(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? "1" : "0";
}

SRV _compare_head(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	return	compare_head_nocase(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? "1" : "0";
}

SRV _compare_tail_case(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	return	compare_tail(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? "1" : "0";
}

SRV _compare_tail(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, "引数の個数が正しくありません。");
	return	compare_tail_nocase(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? "1" : "0";
}

SRV _length(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<1 )
		return	"0";
	return	itos( sjis_strlen(iArguments[0].c_str()) );
}

SRV _is_empty(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<1 )
		return	"1";
	if ( iArguments[0].empty() )
		return	"1";
	else
		return	"0";
}

SRV _is_digit(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<1 || iArguments[0].empty() ) {
		return	"0";
	}

	int dot_count = 0;
	if ( iArguments.size()>=2 ) {
		if ( strstr(iArguments[1].c_str(),"整数") || strcmp(iArguments[1].c_str(),"int") ) {
			dot_count = 1;
		}
	}

	int	i = 0;
	const char* p = iArguments[0].c_str();
	int step = 1;

	static const char zen_pm[] = "＋−";

	if ( p[0] == '-' || p[0] == '+' ) {
		p += 1;
	}
	else if ( (p[0] == zen_pm[0] && p[1] == zen_pm[1]) || (p[0] == zen_pm[2] && p[1] == zen_pm[3]) ) {
		p += 2;
	}

	if ( *p == 0 ) { return "0"; }

	static const char dot_pm[] = "．";

	for ( ; *p ; p += step ) {
		for ( i=0 ; i<20 ; i+=2) {
			if ( p[0]==zen_digit[i] && p[1]==zen_digit[i+1] ) {
				break;
			}
		}
		if ( i<20 ) {
			step = 2;
			continue;
		}

		for ( i=0 ; i<10 ; ++i) {
			if ( p[0]==han_digit[i] ) {
				break;
			}
		}
		if ( i<10 ) {
			step = 1;
			continue;
		}

		if ( p[0]==dot_pm[0] && p[1]==dot_pm[1] ) {
			if ( dot_count == 0 ) {
				dot_count += 1;
				step = 2;
				continue;
			}
		}

		if ( p[0]=='.' ) {
			if ( dot_count == 0 ) {
				dot_count += 1;
				step = 1;
				continue;
			}
		}

		return	"0";
	}
	return	"1";
}

SRV _is_alpha(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()<1 || iArguments[0].empty() )
		return	"0";
	return	arealphabets(iArguments[0]) ? "1" : "0";
}

SRV _zen2han(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( ! iArguments.size() )
		return	SRV(400, "引数の個数が正しくありません。");

	unsigned int flag = 0xffff;
	if ( iArguments.size() >= 2 ) {
		flag = 0;
		if ( iArguments[1].find("アルファベット") != string::npos ) {
			flag |= 0x1;
		}
		if ( iArguments[1].find("数字") != string::npos ) {
			flag |= 0x2;
		}
		if ( iArguments[1].find("記号") != string::npos ) {
			flag |= 0x4;
		}
		if ( iArguments[1].find("カナ") != string::npos ) {
			flag |= 0x8;
		}
	}
	return zen2han_internal(iArguments[0],flag);
}

string zen2han_internal(string &str,unsigned int flag)
{
	char	before[3]="　", after[2]=" ";

	if ( flag & 0x1 ) { //アルファベット
		for (int n=0 ; n<sizeof(han_alpha) ; ++n) {
			before[0]=zen_alpha[n*2];
			before[1]=zen_alpha[n*2+1];
			after[0]=han_alpha[n];
			replace(str, before, after);
		}
	}
	if ( flag & 0x2 ) { //数字
		for (int n=0 ; n<sizeof(han_digit) ; ++n) {
			before[0]=zen_digit[n*2];
			before[1]=zen_digit[n*2+1];
			after[0]=han_digit[n];
			replace(str, before, after);
		}
	}
	if ( flag & 0x4 ) { //記号
		for (int n=0 ; n<sizeof(han_symbol) ; ++n) {
			before[0]=zen_symbol[n*2];
			before[1]=zen_symbol[n*2+1];
			after[0]=han_symbol[n];
			replace(str, before, after);
		}
	}
	if ( flag & 0x8 ) { //カナ
		int n;
		char after2[3] = "  ";
		for (n=0 ; n< (sizeof(han_kana_2)/2) ; ++n) {
			before[0]=zen_kana_2[n*2];
			before[1]=zen_kana_2[n*2+1];
			after2[0]=han_kana_2[n*2];
			after2[1]=han_kana_2[n*2+1];
			replace(str, before, after2);
		}
		for (n=0 ; n<sizeof(han_kana_1) ; ++n) {
			before[0]=zen_kana_1[n*2];
			before[1]=zen_kana_1[n*2+1];
			after[0]=han_kana_1[n];
			replace(str, before, after);
		}
	}
	return	str;
}

SRV _han2zen(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( ! iArguments.size() )
		return	SRV(400, "引数の個数が正しくありません。");

	unsigned int flag = 0xffff;
	if ( iArguments.size() >= 2 ) {
		flag = 0;
		if ( iArguments[1].find("アルファベット") != string::npos ) {
			flag |= 0x1;
		}
		if ( iArguments[1].find("数字") != string::npos ) {
			flag |= 0x2;
		}
		if ( iArguments[1].find("記号") != string::npos ) {
			flag |= 0x4;
		}
		if ( iArguments[1].find("カナ") != string::npos ) {
			flag |= 0x8;
		}
	}

	char	before[2]=" ", after[3]="  ";
	string&	str=iArguments[0];

	if ( flag & 0x1 ) { //アルファベット
		for (int n=0 ; n<sizeof(han_alpha) ; ++n) {
			before[0]=han_alpha[n];
			after[0]=zen_alpha[n*2];
			after[1]=zen_alpha[n*2+1];
			replace(str, before, after);
		}
	}
	if ( flag & 0x2 ) { //数字
		for (int n=0 ; n<sizeof(han_digit) ; ++n) {
			before[0]=han_digit[n];
			after[0]=zen_digit[n*2];
			after[1]=zen_digit[n*2+1];
			replace(str, before, after);
		}
	}
	if ( flag & 0x4 ) { //記号
		for (int n=0 ; n<sizeof(han_symbol) ; ++n) {
			before[0]=han_symbol[n];
			after[0]=zen_symbol[n*2];
			after[1]=zen_symbol[n*2+1];
			replace(str, before, after);
		}
	}
	if ( flag & 0x8) { //カナ
		int n;
		char before2[3] = "  ";
		for (n=0 ; n<(sizeof(han_kana_2)/2) ; ++n) {
			before2[0]=han_kana_2[n*2];
			before2[1]=han_kana_2[n*2+1];
			after[0]=zen_kana_2[n*2];
			after[1]=zen_kana_2[n*2+1];
			replace(str, before2, after);
		}
		for (n=0 ; n<sizeof(han_kana_1) ; ++n) {
			before[0]=han_kana_1[n];
			after[0]=zen_kana_1[n*2];
			after[1]=zen_kana_1[n*2+1];
			replace(str, before, after);
		}
	}

	return	str;
}

SRV _hira2kata(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, "引数の個数が正しくありません。");

	string&	str=iArguments[0];
	for (int i=0 ; str[i]!='\0' ; i+=_ismbblead(str[i])?2:1) {
		for (int j=0 ; j<sizeof(hira) ; j+=2) {
			if ( str[i]==hira[j] && str[i+1]==hira[j+1] ) {
				str[i]=kata[j];
				str[i+1]=kata[j+1];
			}
		}
	}
	return	iArguments[0];
}

SRV _kata2hira(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, "引数の個数が正しくありません。");

	string&	str=iArguments[0];
	for (int i=0 ; str[i]!='\0' ; i+=_ismbblead(str[i])?2:1) {
		for (int j=0 ; j<sizeof(hira) ; j+=2) {
			if ( str[i]==kata[j] && str[i+1]==kata[j+1] ) {
				str[i]=hira[j];
				str[i+1]=hira[j+1];
			}
		}
	}
	return	iArguments[0];
}

SRV _sprintf(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.empty() )
		return	SRV(400, "引数が足りません。");
	return	sprintf(iArguments);
}

SRV _reverse(std::deque<string>& iArguments, std::deque<string>& oValues) {
	if ( iArguments.empty() )
		return	SRV(400, "引数が足りません。");

	string	r;
	const char* p = iArguments[0].c_str();
	while (*p != '\0') {
		const int len = _ismbblead(*p)?2:1;
		r = string(p, len) + r;
		p += len;
	}

	return	r;
}

SRV _at(std::deque<string>& iArguments, std::deque<string>& oValues) {

	if ( iArguments.size()==2 ) {
		const char* p = sjis_at(iArguments.at(0).c_str(), zen2int(iArguments.at(1)));
		return	(p==NULL || *p=='\0') ? "" : string(p, _ismbblead(*p)?2:1);
	}
	//else if ( iArguments.size()==3 ) {
	//}
	else
		return	SRV(400, "引数が正しくありません。");
}



/*
if ( compare_head(theCommand, "tm") ) {
	string	TimeCommands(const string& iCommand, const deque<string>& iArguments);
	return	TimeCommands(theCommand, iArguments);
}
*/
SRV _choice(std::deque<string>& iArguments, std::deque<string>& oValues)
{
	if ( iArguments.size()==0 )
	{
		return	"";
	}
	return iArguments[ random(iArguments.size()) ];
}

SRV _lsimg(std::deque<string>& iArguments, std::deque<string>& oValues)
{
	if (iArguments.size() == 0)
		return "0";
#ifdef WIN32
	WIN32_FIND_DATA wfd;
	string d(iArguments[0]);
	if (!compare_tail(d, "\\")) d += '\\';
	d += '*';
	HANDLE h = FindFirstFile(d.c_str(), &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return "0";
	do {
		string lo(wfd.cFileName);
		std::transform(lo.begin(), lo.end(), lo.begin(), tolower);
		if (compare_tail(lo, ".png") ||
			compare_tail(lo, ".jpg") ||
			compare_tail(lo, ".jpe") ||
			compare_tail(lo, ".jpeg") ||
			compare_tail(lo, ".bmp"))
		{
			oValues.push_back(wfd.cFileName);
		}
	} while (FindNextFile(h, &wfd));
	FindClose(h);
	char s[32];
	sprintf(s, "%d", oValues.size());
	return s;
#else
	// TODO だれかなんとかして
#endif
}

#ifdef WIN32
static inline int mkdir(const char* path, int mode)
{
	return !CreateDirectory(path, NULL);
}
#endif
SRV _mkdir(std::deque<string>& iArguments, std::deque<string>& oValues)
{
	if (iArguments.size() == 0)
		return "";
	if (mkdir(iArguments[0].c_str(), 0777) == 0)
		return "1";
	else
		return "0";
}
