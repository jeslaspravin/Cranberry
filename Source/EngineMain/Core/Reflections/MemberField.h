#pragma once

template <bool IsConst, typename MemberType>
class MemberField;

template <typename MemberType>
class MemberField<false, MemberType>
{
public:
    typedef MemberType (*MemberFieldPtr);

private:
    MemberFieldPtr memberPtr;
public:
    MemberField() : memberPtr(nullptr) {}
    MemberField(const MemberField& otherField)
        : memberPtr(otherField.memberPtr)
    {}
    MemberField(MemberField&& otherField)
        : memberPtr(std::move(otherField.memberPtr))
    {}
    MemberField(const MemberFieldPtr& memberField)
        : memberPtr(memberField)
    {}
    void operator=(const MemberField& otherField)
    {
        memberPtr = otherField.memberPtr;
    }
    void operator=(MemberField&& otherField)
    {
        memberPtr = std::move(otherField.memberPtr);
    }
    void operator=(const MemberFieldPtr& fieldPtr)
    {
        memberPtr = fieldPtr;
    }

    operator bool() const
    {
        return memberPtr != nullptr;
    }

    void set(const MemberType& newValue) const
    {
        *memberPtr = newValue;
    }

    MemberType& get() const
    {
        return *memberPtr;
    }
};

template <typename MemberType>
class MemberField<true, MemberType>
{
public:
    typedef const MemberType(*MemberFieldPtr);

private:
    MemberFieldPtr memberPtr;
public:

    MemberField() : memberPtr(nullptr) {}
    MemberField(const MemberField& otherField)
        : memberPtr(otherField.memberPtr)
    {}
    MemberField(MemberField&& otherField)
        : memberPtr(std::move(otherField.memberPtr))
    {}
    MemberField(const MemberFieldPtr& memberField)
        : memberPtr(memberField)
    {}
    void operator=(const MemberField& otherField)
    {
        memberPtr = otherField.memberPtr;
    }
    void operator=(MemberField&& otherField)
    {
        memberPtr = std::move(otherField.memberPtr);
    }
    void operator=(const MemberFieldPtr& fieldPtr)
    {
        memberPtr = fieldPtr;
    }

    operator bool() const
    {
        return memberPtr != nullptr;
    }

    const MemberType& get() const
    {
        return *memberPtr;
    }
};

template <bool IsConst, typename ClassType, typename MemberType>
class ClassMemberField;

template <typename ClassType,typename MemberType>
class ClassMemberField<false, ClassType, MemberType>
{
public:
    typedef MemberType (ClassType::*MemberFieldPtr);

private:
    MemberFieldPtr memberPtr;
public:
    ClassMemberField() : memberPtr(nullptr) {}
    ClassMemberField(const ClassMemberField& otherField)
        : memberPtr(otherField.memberPtr)
    {}
    ClassMemberField(ClassMemberField&& otherField)
        : memberPtr(std::move(otherField.memberPtr))
    {}
    ClassMemberField(const MemberFieldPtr& memberField)
        : memberPtr(memberField)
    {}
    void operator=(const ClassMemberField& otherField)
    {
        memberPtr = otherField.memberPtr;
    }
    void operator=(ClassMemberField&& otherField)
    {
        memberPtr = std::move(otherField.memberPtr);
    }
    void operator=(const MemberFieldPtr& fieldPtr)
    {
        memberPtr = fieldPtr;
    }

    operator bool() const
    {
        return memberPtr != nullptr;
    }

    void set(ClassType& object, const MemberType& newValue) const
    {
        object.*memberPtr = newValue;
    }

    void set(ClassType* object, const MemberType& newValue) const
    {
        object->*memberPtr = newValue;
    }

    MemberType& get(ClassType& object) const
    {
        return object.*memberPtr;
    }

    MemberType& get(ClassType* object) const
    {
        return object->*memberPtr;
    }

    const MemberType& get(const ClassType& object) const
    {
        return object.*memberPtr;
    }

    const MemberType& get(const ClassType* object) const
    {
        return object->*memberPtr;
    }
};
template <typename ClassType, typename MemberType>
class ClassMemberField<true, ClassType, MemberType>
{
public:
    typedef const MemberType(ClassType::*MemberFieldPtr);

private:
    MemberFieldPtr memberPtr;
public:

    ClassMemberField() : memberPtr(nullptr) {}
    ClassMemberField(const ClassMemberField& otherField)
        : memberPtr(otherField.memberPtr)
    {}
    ClassMemberField(ClassMemberField&& otherField)
        : memberPtr(std::move(otherField.memberPtr))
    {}
    ClassMemberField(const MemberFieldPtr& memberField)
        : memberPtr(memberField)
    {}
    void operator=(const ClassMemberField& otherField)
    {
        memberPtr = otherField.memberPtr;
    }
    void operator=(ClassMemberField&& otherField)
    {
        memberPtr = std::move(otherField.memberPtr);
    }
    void operator=(const MemberFieldPtr& fieldPtr)
    {
        memberPtr = fieldPtr;
    }

    operator bool() const
    {
        return memberPtr != nullptr;
    }

    const MemberType& get(const ClassType& object) const
    {
        return object.*memberPtr;
    }

    const MemberType& get(const ClassType* object) const
    {
        return object->*memberPtr;
    }
};