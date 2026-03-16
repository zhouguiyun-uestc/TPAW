#include "MPI_Communicate.h"
#include "MPI_serialize.h"
#include <mpi.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <iterator>
#include <zstd.h>
void send(LocalSolution& localSol, int toRank)
{
    //object to a stream
    std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
    ss.unsetf(std::ios_base::skipws);
    {
        cereal::BinaryOutputArchive archive(ss);
        archive(localSol);
    }

    std::string rawData = ss.str();
    size_t const rawSize = rawData.size();

    size_t const maxCompressedSize = ZSTD_compressBound(rawSize);
    std::vector<char> compressedBuffer(maxCompressedSize);

    //Compression level: 1 (fastest)
    size_t const compressedSize = ZSTD_compress(compressedBuffer.data(), maxCompressedSize,
        rawData.data(), rawSize, 1);

    if (ZSTD_isError(compressedSize)) {
        std::cerr << "Zstd Error: " << ZSTD_getErrorName(compressedSize) << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    localSol.transferredByte = compressedSize;

    int tag = 0;
    MPI_Send(compressedBuffer.data(), (int)compressedSize, MPI_CHAR, toRank, tag, MPI_COMM_WORLD);

    //string to array
    /*std::vector<char> msg;
    std::copy(std::istream_iterator< char >(ss), std::istream_iterator< char >(), std::back_inserter(msg));

    localSol.transferredByte = msg.size();
    int tag = 0;
	MPI_Send(msg.data(), msg.size(), MPI_CHAR, toRank, tag, MPI_COMM_WORLD);*/
}

LocalSolution receive(int fromRank)
{
    LocalSolution localSol;

    //determin the message size
    MPI_Status status;
    if (fromRank == -1)
        fromRank = MPI_ANY_SOURCE;
    MPI_Probe(fromRank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    int compressedSize;
    MPI_Get_count(&status, MPI_CHAR, &compressedSize);
    if (compressedSize <= 0) 
    {
        std::cout << "message of zero size" << std::endl;
        return localSol;
    }
    int receive_Id = status.MPI_SOURCE;

    std::vector<char> compressedBuffer(compressedSize);
    MPI_Recv(compressedBuffer.data(), compressedSize, MPI_CHAR, receive_Id, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    //theoretical decompressed size
    size_t const rSize = ZSTD_getFrameContentSize(compressedBuffer.data(), compressedSize);
    if (rSize == ZSTD_CONTENTSIZE_ERROR || rSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        std::cerr << "Zstd: Could not determine original size" << std::endl;
        return localSol;
    }
    std::vector<char> rawBuffer(rSize);

    //actual decompressed size
    size_t const dSize = ZSTD_decompress(rawBuffer.data(),rSize, compressedBuffer.data(), compressedSize);

    if (ZSTD_isError(dSize)) {
        std::cerr << "Zstd Decompress Error: " << ZSTD_getErrorName(dSize) << std::endl;
        return localSol;
    }
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    ss.write(rawBuffer.data(), rawBuffer.size());
    {
        cereal::BinaryInputArchive archive(ss);
        archive(localSol);
    }

    localSol.transferredByte = compressedSize;
    return localSol;

    //int msg_size;
    //MPI_Get_count(&status, MPI_CHAR, &msg_size);
    //if (msg_size == 0)
    //{
    //    std::cout << "message of zero size" << std::endl;
    //    return localSol;
    //}

    //int receive_Id = status.MPI_SOURCE;

    ////receive into a char array
    //std::vector<char> msg(msg_size);    
    //MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    //int error_code = MPI_Recv(msg.data(), msg_size, MPI_CHAR, receive_Id, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //if (error_code != MPI_SUCCESS) {
    //    //char error_string[BUFSIZ];
    //    //int length_of_error_string, error_class;
    //    //MPI_Error_class(error_code, &error_class);
    //    //MPI_Error_string(error_class, error_string, &length_of_error_string);
    //    std::cout << "Error occured during MPI_Recv:"<<error_code << std::endl;
    //    MPI_Abort(MPI_COMM_WORLD, error_code);
    //}

    ////deserialize into a stream
    //std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
    //ss.unsetf(std::ios_base::skipws);
    //ss.write(msg.data(), msg_size);

    ////stream to object
    //cereal::BinaryInputArchive archive(ss);
    //archive(localSol);

    //localSol.transferredByte = msg_size;
    //
    //return localSol;
}