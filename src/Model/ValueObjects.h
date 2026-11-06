#ifndef VALUE_OBJECTS_H
#define VALUE_OBJECTS_H

#include <stdexcept>
#include <string>
#include <algorithm>
#include <cctype>

namespace ReservationDomain
{

    inline bool isBlank(const std::string &value)
    {
        return value.empty() ||
               std::all_of(
                   value.begin(),
                   value.end(),
                   [](unsigned char character)
                   {
                       return std::isspace(character);
                   });
    }

    class RequestId
    {
    private:
        std::string value;

    public:
        explicit RequestId(const std::string &value)
            : value(value)
        {
            if (isBlank(value))
            {
                throw std::invalid_argument(
                    "Request ID cannot be empty");
            }
        }

        const std::string &getValue() const
        {
            return value;
        }
    };

    class ProductId
    {
    private:
        std::string value;

    public:
        explicit ProductId(const std::string &value)
            : value(value)
        {
            if (isBlank(value))
            {
                throw std::invalid_argument(
                    "Product ID cannot be empty");
            }
        }

        const std::string &getValue() const
        {
            return value;
        }
    };

    class DepartmentId
    {
    private:
        std::string value;

    public:
        explicit DepartmentId(const std::string &value)
            : value(value)
        {
            if (isBlank(value))
            {
                throw std::invalid_argument(
                    "Department ID cannot be empty");
            }
        }

        const std::string &getValue() const
        {
            return value;
        }
    };

    class Quantity
    {
    private:
        int value;

    public:
        explicit Quantity(int value)
            : value(value)
        {
            if (value <= 0)
            {
                throw std::invalid_argument(
                    "Requested quantity must be greater than zero");
            }
        }

        int getValue() const
        {
            return value;
        }
    };

    class AvailableQuantity
    {
    private:
        int value;

    public:
        explicit AvailableQuantity(int value)
            : value(value)
        {
            if (value < 0)
            {
                throw std::invalid_argument(
                    "Available quantity cannot be negative");
            }
        }

        int getValue() const
        {
            return value;
        }
        bool operator==(const AvailableQuantity &other) const
        {
            return value == other.value;
        }
    };

}

#endif
