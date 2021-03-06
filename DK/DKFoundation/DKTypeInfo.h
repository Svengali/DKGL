//
//  File: DKTypeInfo.h
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2004-2016 Hongtae Kim. All rights reserved.
//

#pragma once
#include <typeinfo>
#include "../DKInclude.h"
#include "DKString.h"

namespace DKFoundation
{
	/// a wrapper class for std::type_info
	/// You can use std::type_info directly.
	class DKGL_API DKTypeInfo
	{
	public:
		DKTypeInfo();
		DKTypeInfo(const DKTypeInfo& ti);
		DKTypeInfo(const std::type_info&);
		~DKTypeInfo();

		bool IsValid() const;

		DKString Name() const;
		bool Before(const DKTypeInfo& rhs) const;
		operator const std::type_info& () const;

		DKTypeInfo& operator = (const DKTypeInfo& ti);

		bool operator == (const DKTypeInfo& rhs) const;
		bool operator != (const DKTypeInfo& rhs) const;
		bool operator < (const DKTypeInfo& rhs) const;
		bool operator > (const DKTypeInfo& rhs) const;
		bool operator <= (const DKTypeInfo& rhs) const;
		bool operator >= (const DKTypeInfo& rhs) const;

	private:
		const std::type_info* info;
	};
}
