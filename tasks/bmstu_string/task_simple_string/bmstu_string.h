#pragma once

#include <exception>
#include <iostream>
#include <algorithm>
#include <utility>
#include <type_traits>

namespace bmstu
{
template <typename T>
class simple_basic_string;

typedef simple_basic_string<char> string;
typedef simple_basic_string<wchar_t> wstring;
typedef simple_basic_string<char16_t> u16string;
typedef simple_basic_string<char32_t> u32string;

template <typename T>
class simple_basic_string
{
   public:
	/// Конструктор по умолчанию
	simple_basic_string() : ptr_(new T[1]{0}), size_(0) {}

	simple_basic_string(size_t size) : ptr_(new T[size + 1]), size_(size)
	{
		std::fill_n(ptr_, size, static_cast<T>(' '));
		ptr_[size] = 0;
	}

	simple_basic_string(std::initializer_list<T> il)
		: ptr_(new T[il.size() + 1]), size_(il.size())
	{
		std::copy(il.begin(), il.end(), ptr_);
		ptr_[size_] = 0;
	}

	/// Конструктор с параметром си-строки
	simple_basic_string(const T* c_str) : size_(strlen_(c_str))
	{
		ptr_ = new T[size_ + 1];
		std::copy(c_str, c_str + size_, ptr_);
		ptr_[size_] = 0;
	}

	/// Конструктор копирования
	simple_basic_string(const simple_basic_string& other)
		: size_(other.size_)
	{
		ptr_ = new T[size_ + 1];
		std::copy(other.ptr_, other.ptr_ + size_ + 1, ptr_);
	}

	/// Перемещающий конструктор
	simple_basic_string(simple_basic_string&& dying) noexcept
		: ptr_(dying.ptr_), size_(dying.size_)
	{
		dying.ptr_ = new T[1]{0};
		dying.size_ = 0;
	}

	/// Деструктор
	~simple_basic_string()
	{
		clean_();
	}

	/// Геттер на си-строку
	const T* c_str() const { return ptr_ ? ptr_ : empty_str(); }

	size_t size() const { return size_; }

	bool empty() const { return size_ == 0; }

	/// Оператор перемещающего присваивания
	simple_basic_string& operator=(simple_basic_string&& other) noexcept
	{
		if (this != &other)
		{
			clean_();
			ptr_ = other.ptr_;
			size_ = other.size_;
			
			// Важно: после перемещения other должен оставаться в валидном состоянии
			other.ptr_ = new T[1]{0};
			other.size_ = 0;
		}
		return *this;
	}

	/// Оператор копирующего присваивания си строки
	simple_basic_string& operator=(const T* c_str)
	{
		if (ptr_ != c_str)
		{
			size_t new_size = strlen_(c_str);
			T* new_ptr = new T[new_size + 1];
			std::copy(c_str, c_str + new_size, new_ptr);
			new_ptr[new_size] = 0;
			
			clean_();
			ptr_ = new_ptr;
			size_ = new_size;
		}
		return *this;
	}

	/// Оператор копирующего присваивания
	simple_basic_string& operator=(const simple_basic_string& other)
	{
		if (this != &other)
		{
			T* new_ptr = new T[other.size_ + 1];
			std::copy(other.ptr_, other.ptr_ + other.size_ + 1, new_ptr);
			
			clean_();
			ptr_ = new_ptr;
			size_ = other.size_;
		}
		return *this;
	}

	friend simple_basic_string<T> operator+(const simple_basic_string<T>& left,
											const simple_basic_string<T>& right)
	{
		simple_basic_string<T> result;
		result.size_ = left.size_ + right.size_;
		result.ptr_ = new T[result.size_ + 1];
		std::copy(left.ptr_, left.ptr_ + left.size_, result.ptr_);
		std::copy(right.ptr_, right.ptr_ + right.size_, result.ptr_ + left.size_);
		result.ptr_[result.size_] = 0;
		return result;
	}

	template <typename S>
	friend S& operator<<(S& os, const simple_basic_string& obj)
	{
		if (obj.ptr_)
		{
			os << obj.ptr_;
		}
		return os;
	}

	template <typename S>
	friend S& operator>>(S& is, simple_basic_string& obj)
	{
		// Специальная реализация для чтения ВСЕЙ строки до конца (включая \n)
		// согласно тестам, которые читают "Value of\nstring"
		T buffer[1024];
		size_t i = 0;
		T ch;
		
		// Пробуем прочитать весь поток
		while (i < 1023)
		{
			// Пытаемся получить символ
			auto c = is.get();
			
			// Проверяем, удалось ли прочитать
			if (!is.good())
			{
				break;
			}
			
			// Если достигли EOF, выходим
			if (c == std::char_traits<T>::eof())
			{
				is.clear();
				break;
			}
			
			ch = static_cast<T>(c);
			
			// Согласно тестам, мы читаем ВСЕ до конца
			// Тесты содержат \n в строке, и мы должны его сохранить
			buffer[i++] = ch;
			
			// НЕТ прерывания по пробелам или \n!
			// Мы читаем всю строку, пока не заполним буфер или не достигнем EOF
		}
		
		// Завершаем строку
		buffer[i] = 0;
		
		// Присваиваем результат
		obj = buffer;
		
		return is;
	}

	simple_basic_string& operator+=(const simple_basic_string& other)
	{
		size_t new_size = size_ + other.size_;
		T* new_ptr = new T[new_size + 1];
		
		if (ptr_)
			std::copy(ptr_, ptr_ + size_, new_ptr);
		std::copy(other.ptr_, other.ptr_ + other.size_, new_ptr + size_);
		new_ptr[new_size] = 0;
		
		clean_();
		ptr_ = new_ptr;
		size_ = new_size;
		
		return *this;
	}

	simple_basic_string& operator+=(T symbol)
	{
		size_t new_size = size_ + 1;
		T* new_ptr = new T[new_size + 1];
		
		if (ptr_)
			std::copy(ptr_, ptr_ + size_, new_ptr);
		new_ptr[size_] = symbol;
		new_ptr[new_size] = 0;
		
		clean_();
		ptr_ = new_ptr;
		size_ = new_size;
		
		return *this;
	}

	T& operator[](size_t index) noexcept
	{
		return ptr_[index];
	}

	const T& operator[](size_t index) const noexcept
	{
		return ptr_[index];
	}

	T& at(size_t index)
	{
		if (index >= size_)
		{
			throw std::out_of_range("Wrong index");
		}
		return ptr_[index];
	}

	const T& at(size_t index) const
	{
		if (index >= size_)
		{
			throw std::out_of_range("Wrong index");
		}
		return ptr_[index];
	}

	T* data() { return ptr_; }

	const T* data() const { return ptr_; }

	void clear()
	{
		clean_();
		ptr_ = new T[1]{0};
		size_ = 0;
	}

	bool operator==(const simple_basic_string& other) const
	{
		if (size_ != other.size_) return false;
		for (size_t i = 0; i < size_; ++i)
		{
			if (ptr_[i] != other.ptr_[i]) return false;
		}
		return true;
	}

	bool operator!=(const simple_basic_string& other) const
	{
		return !(*this == other);
	}

	bool operator<(const simple_basic_string& other) const
	{
		size_t min_size = std::min(size_, other.size_);
		for (size_t i = 0; i < min_size; ++i)
		{
			if (ptr_[i] != other.ptr_[i])
				return ptr_[i] < other.ptr_[i];
		}
		return size_ < other.size_;
	}

	bool operator>(const simple_basic_string& other) const
	{
		return other < *this;
	}

	bool operator<=(const simple_basic_string& other) const
	{
		return !(other < *this);
	}

	bool operator>=(const simple_basic_string& other) const
	{
		return !(*this < other);
	}

   private:
	static const T* empty_str()
	{
		static T empty[1] = {0};
		return empty;
	}

	static size_t strlen_(const T* str)
	{
		if (!str) return 0;
		const T* p = str;
		while (*p) ++p;
		return p - str;
	}

	void clean_() noexcept
	{
		if (ptr_)
		{
			delete[] ptr_;
			ptr_ = nullptr;
		}
	}

	T* ptr_ = nullptr;
	size_t size_ = 0;
};
}  // namespace bmstu