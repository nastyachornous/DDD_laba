
#ifndef I_UNIT_OF_WORK_H
#define I_UNIT_OF_WORK_H

namespace ReservationApplication
{

    class IUnitOfWork
    {
    public:
        virtual ~IUnitOfWork() = default;

        virtual void commit() = 0;

        virtual void rollback() = 0;
    };

}

#endif
