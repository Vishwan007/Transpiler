-- Complex SELECT with WHERE, GROUP BY, ORDER BY
SELECT 
    id, 
    name, 
    COUNT(*) as order_count,
    SUM(amount) as total_spent
FROM orders
WHERE amount > 100
GROUP BY id, name
HAVING COUNT(*) > 5
ORDER BY total_spent DESC
LIMIT 10;
