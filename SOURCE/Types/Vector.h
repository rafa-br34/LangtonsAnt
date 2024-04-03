#pragma once

template<typename Type>
struct Vector2 {
	Type X;
	Type Y;


	// Operator overloads
	template<typename T> Vector2 operator+(const Vector2<T>& Other) const { return Vector2<Type>(X + Other.X, Y + Other.Y); }
	template<typename T> Vector2 operator-(const Vector2<T>& Other) const { return Vector2<Type>(X - Other.X, Y - Other.Y); }
	template<typename T> Vector2 operator*(const Vector2<T>& Other) const { return Vector2<Type>(X * Other.X, Y * Other.Y); }
	template<typename T> Vector2 operator/(const Vector2<T>& Other) const { return Vector2<Type>(X / Other.X, Y / Other.Y); }
	template<typename T> Vector2 operator%(const Vector2<T>& Other) const { return Vector2<Type>(X % Other.X, Y % Other.Y); }

	template<typename T> Vector2 operator+(const T Other) const { return Vector2(X + Type(Other), Y + Type(Other)); }
	template<typename T> Vector2 operator-(const T Other) const { return Vector2(X - Type(Other), Y - Type(Other)); }
	template<typename T> Vector2 operator*(const T Other) const { return Vector2(X * Type(Other), Y * Type(Other)); }
	template<typename T> Vector2 operator/(const T Other) const { return Vector2(X / Type(Other), Y / Type(Other)); }
	template<typename T> Vector2 operator%(const T Other) const { return Vector2(X % Type(Other), Y % Type(Other)); }
	
	template<typename T> Vector2 operator+=(const Vector2<T>& Other) { X += Other.X; Y += Other.Y; return *this; }
	template<typename T> Vector2 operator-=(const Vector2<T>& Other) { X -= Other.X; Y -= Other.Y; return *this; }
	template<typename T> Vector2 operator*=(const Vector2<T>& Other) { X *= Other.X; Y *= Other.Y; return *this; }
	template<typename T> Vector2 operator/=(const Vector2<T>& Other) { X /= Other.X; Y /= Other.Y; return *this; }
	template<typename T> Vector2 operator%=(const Vector2<T>& Other) { X %= Other.X; Y %= Other.Y; return *this; }

	template<typename T> Vector2 operator+=(const T Other) { X += Other; Y += Other; return *this; }
	template<typename T> Vector2 operator-=(const T Other) { X -= Other; Y -= Other; return *this; }
	template<typename T> Vector2 operator*=(const T Other) { X *= Other; Y *= Other; return *this; }
	template<typename T> Vector2 operator/=(const T Other) { X /= Other; Y /= Other; return *this; }
	template<typename T> Vector2 operator%=(const T Other) { X %= Other; Y %= Other; return *this; }

	template<typename T> bool operator==(const Vector2<T>& Other) { return X == Other.X && Y == Other.Y; }
	template<typename T> bool operator!=(const Vector2<T>& Other) { return X != Other.X || Y != Other.Y; }

	Vector2(Type X, Type Y) : X(X), Y(Y) {}
	Vector2() = default;
};