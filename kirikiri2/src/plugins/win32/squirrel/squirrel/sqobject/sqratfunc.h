#ifndef SQRATFUNC_H
#define SQRATFUNC_H

#include "sqobjectinfo.h"

/**
 * sqobject::sqobject を継承したオブジェクト用に sqrat あわせでテンプレートを作成
 */

namespace sqobject {

	/**
	 * 引数なしコンストラクタ用アロケータ
	 */
	template <class C>
	class DefaultConstructor {
	public:
		static SQRESULT New(HSQUIRRELVM vm) {
			C *instance = new C();
			if (instance) {
				instance->initSelf(vm, 1);
				sq_setinstanceup(vm, 1, instance);
				sq_setreleasehook(vm, 1, &Delete);
				return SQ_OK;
			} else {
				return SQ_ERROR;
			}
		}
		
		static SQInteger Copy(HSQUIRRELVM vm, SQInteger idx, const void* value) {
			C* instance = new C(*static_cast<const C*>(value));
			if (instance) {
				instance->initSelf(vm, idx);
				sq_setinstanceup(vm, idx, instance);
				sq_setreleasehook(vm, idx, &Delete);
				return SQ_OK;
			} else {
				return SQ_ERROR;
			}
		}
		
		static SQRESULT Delete(SQUserPointer up, SQInteger size) {
			C* self = (C*)up;
			if (self){ 
				self->destructor();
				delete self;
			}
			return SQ_OK;
		}
	};
	
	/**
	 * HSQUIRRELVMを引数にとるコンストラクタ用のアロケータ
	 */
	template <class C>
	class VMConstructor : public DefaultConstructor<C> {
	public:
		static SQRESULT New(HSQUIRRELVM vm) {
			C *instance = new C(vm);
			if (instance) {
				instance->initSelf(vm);
				sq_setinstanceup(vm, 1, instance);
				sq_setreleasehook(vm, 1, &Delete);
				return SQ_OK;
			} else {
				return SQ_ERROR;
			}
		}
	};
	
	/**
	 * コンストラクタなしオブジェクト用アロケータ。newすると例外
	 */
	template <class C>
	class NOConstructor : public DefaultConstructor<C> {
	public:
		static SQRESULT New(HSQUIRRELVM v) {
			return sq_throwerror(v, "can't create instance");
		}
	};

	// ---------------------------------------------------------------------------

	/**
	 * _get/_set に割り込みする処理。
	 * sqrat のプロパティ処理が失敗した場合はインスタンスの _set/_get を
	 * 呼び出すようにする
	 */
	template<class C>
	struct OverrideSetGet {
		
		static SQInteger _get(HSQUIRRELVM vm) {
			// Find the get method in the get table
			sq_push(vm, 2);
			if (SQ_FAILED( sq_get(vm,-2) )) {
				C* ptr = NULL;
				sq_getinstanceup(vm, 1, (SQUserPointer*)&ptr, NULL);
				return ptr->_get(vm);
			}
			
			// push 'this'
			sq_push(vm, 1);
			
			// Call the getter
			sq_call(vm, 1, true, Sqrat::ErrorHandling::IsEnabled());
			return 1;
		}

		static SQInteger _set(HSQUIRRELVM vm) {
			// Find the set method in the set table
			sq_push(vm, 2);
			if (SQ_FAILED( sq_get(vm,-2) )) {
				C* ptr = NULL;
				sq_getinstanceup(vm, 1, (SQUserPointer*)&ptr, NULL);
				return ptr->_set(vm);
			}
			
			// push 'this'
			sq_push(vm, 1);
			sq_push(vm, 3);
			
			// Call the setter
			sq_call(vm, 2, false, Sqrat::ErrorHandling::IsEnabled());
			
			return 0;
		}

		static void Func(HSQUIRRELVM vm) {
			// push the class
			sq_pushobject(vm, Sqrat::ClassType<C>::ClassObject());
			
			// sqrat の機能をオーバライドする

			// override _set
			sq_pushstring(vm, _SC("_set"), -1);
			sq_pushobject(vm, Sqrat::ClassType<C>::SetTable()); // Push the set table as a free variable
			sq_newclosure(vm, _set, 1);
			sq_newslot(vm, -3, false);
			// override _get
			sq_pushstring(vm, _SC("_get"), -1);
			sq_pushobject(vm, Sqrat::ClassType<C>::GetTable()); // Push the get table as a free variable
			sq_newclosure(vm, _get, 1);
			sq_newslot(vm, -3, false);

			// _get / _set で登録したものは後から参照できないので、
			// スクリプトの継承先でオーバライドするときのため別名で登録しておく

			// override _set
			sq_pushstring(vm, _SC("set"), -1);
			sq_pushobject(vm, Sqrat::ClassType<C>::SetTable()); // Push the set table as a free variable
			sq_newclosure(vm, _set, 1);
			sq_newslot(vm, -3, false);
			// override _get
			sq_pushstring(vm, _SC("get"), -1);
			sq_pushobject(vm, Sqrat::ClassType<C>::GetTable()); // Push the get table as a free variable
			sq_newclosure(vm, _get, 1);
			sq_newslot(vm, -3, false);
			sq_pop(vm, 1);
		}
	};
	
}; // namespace

// ----------------------------------------------
// ファンクション登録用マクロ
// ----------------------------------------------

#define SQFUNC(Class, Name)  cls.Func(_SC(#Name), &Class::Name)
#define SQSFUNC(Class, Name) cls.StaticFunc(_SC(#Name), &Class::Name)
#define SQVFUNC(Class, Name) cls.VarArgFunc(_SC(#Name), &Class::Name)

#endif
