#[derive(Debug, Clone, PartialEq)]
pub struct Message<T> {
    pub index: u8,
    pub data: T,
}
