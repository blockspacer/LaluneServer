#pragma once

namespace LexicalEntryValue
{
	enum value_type_enum
	{
		int_value_type,
		uint_value_type,
		uint_value_type_display_as_hex,
		uint16_value_type_display_as_hex,
		string_value_type,
		bytes_value_type_no_display,
		bytes_value_type_display_as_hex,
		double_value_type,
		//����enumֵ���ܲ��м䣬������ǰ�����ݾͶ�ʧЧ�ˡ�����Ӻ���
		wstring_value_type,
		undefined_value_type = 100 //12.08.01���ӣ���Ҫ���ڴ�������µ�Ĭ��ֵ
	};
}