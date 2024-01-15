#pragma once

template<typename Type>
struct Vector2 {
	Type X;
	Type Y;


	// Operator overloads
	Vector2 operator+(const Vector2& Other) const { return Vector2(this->X + Other.X, this->Y + Other.Y); }
	Vector2 operator-(const Vector2& Other) const { return Vector2(this->X - Other.X, this->Y - Other.Y); }
	Vector2 operator*(const Vector2& Other) const { return Vector2(this->X * Other.X, this->Y * Other.Y); }
	Vector2 operator/(const Vector2& Other) const { return Vector2(this->X / Other.X, this->Y / Other.Y); }
	Vector2 operator%(const Vector2& Other) const { return Vector2(this->X % Other.X, this->Y % Other.Y); }
	
	Vector2 operator+=(const Vector2& Other) { this->X += Other.X; this->Y += Other.Y; return *this; }
	Vector2 operator-=(const Vector2& Other) { this->X -= Other.X; this->Y -= Other.Y; return *this; }
	Vector2 operator*=(const Vector2& Other) { this->X *= Other.X; this->Y *= Other.Y; return *this; }
	Vector2 operator/=(const Vector2& Other) { this->X /= Other.X; this->Y /= Other.Y; return *this; }
	Vector2 operator%=(const Vector2& Other) { this->X %= Other.X; this->Y %= Other.Y; return *this; }


	Vector2(Type X, Type Y) { this->X = X; this->Y = Y; }
	Vector2() = default;
};