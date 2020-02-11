# AtomicAsset<T>

Atomic assets are an asset type which implement a transaction pattern to help protect
against issues accessing the asset in separate threads.

> Although Simulant doesn't give any guarantees about thread-safety, the API is designed
> with thread-safety in mind and some primitives provide locking functionality. Atomic
> assets are one of those things.

## Read and Read-Write Transactions

When you want to access the properties of an AtomicAsset you have 3 options:

 - Don't use transactions at all. Some properties are readable directly from the
 asset from wherever, but accessing properties one after the other give no guarantees
 about the consistency of the data.
 - Start a read transaction. Read transactions will block read-write transactions from 
 committing their changes, although you won't be able to manipulate any data with them.
 - Start a read-write transaction. These transactions allow you to manipulate the asset
 through the transaction object. Read-write transactions are exclusive throughout their lifetime.
  
## Example
 
 ```
 auto txn = texture->begin_transaction(ASSET_TRANSACTION_READ_WRITE);
 
 // Do stuff with txn
 
 txn.commit();  // Update "texture"
 ```
